#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>

#include "common/constants.h"
#include "common/util.h"
#include "operations.h"
#include "queue.h"
#include "session.h"

int run_session(struct Request *request, int id) {

  // Open request pipe for writing
  // This waits for someone to open it for reading
  int req_pipe = open(request->request_pipe_name, O_RDONLY);
  if (req_pipe == -1) {
    fprintf(stderr, "[ERR]:Session failed opening request pipe: %s\n",
            strerror(errno));
    return 1;
  }

  // Open response pipe for reading
  // This waits for someone to open it for writing
  int resp_pipe = open(request->response_pipe_name, O_WRONLY);
  if (resp_pipe == -1) {
    fprintf(stderr, "[ERR]:Session failed opening response failed: %s\n",
            strerror(errno));
    return 1;
  }

  send_msg(resp_pipe, &id, sizeof(int));

  char code;
  int session_id;

  while (1) {
    unsigned int event_id;
    int ret;

    get_msg(req_pipe, &code, sizeof(char));
    get_msg(req_pipe, &session_id, sizeof(int));

    switch (code) {
    case OP_QUIT:
      // Unlink response pipe
      if (unlink(request->response_pipe_name) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n",
                request->response_pipe_name, strerror(errno));
        return (1);
      }
      // Unlink request pipe
      if (unlink(request->request_pipe_name) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n",
                request->request_pipe_name, strerror(errno));
        return (1);
      }
      return 0;

    case OP_CREATE:
      size_t num_rows, num_cols;
      get_msg(req_pipe, &event_id, sizeof(unsigned int));
      get_msg(req_pipe, &num_rows, sizeof(size_t));
      get_msg(req_pipe, &num_cols, sizeof(size_t));
      ret = ems_create(event_id, num_rows, num_cols);
      send_msg(resp_pipe, &ret, sizeof(int));
      break;

    case OP_RESERVE:
      size_t num_seats;
      size_t *xs = malloc(sizeof(size_t) * num_seats);
      size_t *ys = malloc(sizeof(size_t) * num_seats);
      get_msg(req_pipe, &event_id, sizeof(unsigned int));
      get_msg(req_pipe, &num_seats, sizeof(size_t));
      get_msg(req_pipe, xs, sizeof(size_t) * num_seats);
      get_msg(req_pipe, ys, sizeof(size_t) * num_seats);
      ret = ems_reserve(event_id, num_seats, xs, ys);
      send_msg(resp_pipe, &ret, sizeof(int));
      break;

    case OP_SHOW:
      get_msg(req_pipe, &event_id, sizeof(unsigned int));
      ems_show(resp_pipe, event_id);
      break;

    case OP_LIST_EVENTS:
      ems_list_events(resp_pipe);
    }
  }
}

void *run_thread(void *args) {
  struct Arguments *arguments = (struct Arguments *)args;
  struct Request *new_request = NULL;

  // create the Set with the signal to be blocked
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  // Block the signal SIGUSR1
  if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) {
    fprintf(stderr, "[ERR]: failed masking SIG_BLOCK: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Locks the queue to retrieve a request
    if (pthread_mutex_lock(&arguments->queue->mutex) != 0) {
      fprintf(stderr, "[ERR]: failed locking a mutex: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }

    // If the queue is empty waits for a signal that a request was made
    while (arguments->queue->head == NULL) {
      pthread_cond_wait(arguments->cond, &arguments->queue->mutex);
    }

    // Fetchs a Request from the Queue
    new_request = pop_request(arguments->queue);

    // Unlocks the Queue
    if (pthread_mutex_unlock(&arguments->queue->mutex) != 0) {
      fprintf(stderr, "[ERR]: failed unlocking a mutex: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }

    // Starts the Session with the new client
    if (new_request != NULL) {
      printf("Thread %u: running pipe: %s\n", arguments->session_id,
             new_request->request_pipe_name);
      if (run_session(new_request, arguments->session_id) == END_THREAD) {
        break;
      }
    }
  }

  pthread_exit(NULL);
}
