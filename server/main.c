#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/constants.h"
#include "common/io.h"
#include "common/util.h"
#include "operations.h"
#include "queue.h"
#include "session.h"

int sigusr1_flag = 0;
int sigint_flag = 0;

static void sig_handler(int sig) {
  if (sig == SIGUSR1) {
    if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
      exit(EXIT_FAILURE);
    }
    sigusr1_flag++;
    fprintf(stderr, "Caught SIGUSR1 (%d)\n", sigusr1_flag);
    return; // Resume execution at point of interruption
  }
  if (sig == SIGINT) {
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
      exit(EXIT_FAILURE);
    }
    sigint_flag++;
    fprintf(stderr, "Caught SIGINT (%d)\n", sigint_flag);
    return; // Resume execution at point of interruption
  }
}

int main(int argc, char *argv[]) {

  // Define the routine for the signal SIGUSR1
  if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
    exit(EXIT_FAILURE);
  }

  // Define the routine for the signal SIGINT (CTRL+C)
  if (signal(SIGINT, sig_handler) == SIG_ERR) {
    exit(EXIT_FAILURE);
  }

  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }

  char *endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  // Intialize server, create worker threads

  char *fifo_pathname = argv[1];

  if (unlink(fifo_pathname) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", fifo_pathname,
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Create server pipe
  if (mkfifo(fifo_pathname, 0640) != 0) {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Open server pipe for reading
  // This waits for someone to open it for writing
  int server_pipe = open(fifo_pathname, O_RDONLY);
  if (server_pipe == -1) {
    fprintf(stderr, "[ERR]:Server failed opening server_pipe: %s\n",
            strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Starts the queue
  struct Queue *queue = create_queue();
  if (!queue) {
    fprintf(stderr, "[ERR]: queue initialize failed: %s\n", strerror(errno));
  }

  // Data structs to handle the worker threads
  pthread_t *threads =
      malloc((unsigned long)MAX_SESSION_COUNT * sizeof(pthread_t));
  struct Arguments *arguments_list =
      (struct Arguments *)malloc(MAX_SESSION_COUNT * sizeof(struct Arguments));

  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

  // Create the worker Threads
  for (int i = 0; i < MAX_SESSION_COUNT; i++) {
    arguments_list[i].queue = queue;
    arguments_list[i].cond = &cond;
    arguments_list[i].session_id = i;
    pthread_create(&threads[i], NULL, run_thread, (void *)&arguments_list[i]);
  }

  while (1) {
    char code;
    ssize_t ret = 0;
    if (sigusr1_flag > 0) {
      // list_event(); //TO DO create this function
      ems_show_events();
      sigusr1_flag--;
    }
    if (sigint_flag > 0) {
      // Close Server

      close(server_pipe);
      free_queue(queue);
      free(threads);
      free(arguments_list);
      return ems_terminate();
    }
    if(server_pipe != -1){
      ret = read(server_pipe, &code, sizeof(char));
    }
    if ( ret < 0) {
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
    // If there is no client to request a session close and open the server pipe
    // to wait for other client
    if (ret == 0) {
      close(server_pipe);
      server_pipe = open(fifo_pathname, O_RDONLY);
      if (server_pipe == -1) {
        if(errno == EINTR){
          continue;
        }
        fprintf(stderr, "[ERR]:Server failed opening server_pipe: %s\n",
                strerror(errno));
        exit(EXIT_FAILURE);
      }
    }
    else if (code == OP_SETUP) {
      char request_pipe_name[NAME_LEN], response_pipe_name[NAME_LEN];

      // Locks the queue to create a request
      if (pthread_mutex_lock(&queue->mutex) != 0) {
        fprintf(stderr, "[ERR]: failed locking a mutex: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }

      // Read both pipe names
      get_msg(server_pipe, request_pipe_name, NAME_LEN);
      get_msg(server_pipe, response_pipe_name, NAME_LEN);

      // Create the new client Request and appends it to the Queue
      struct Request *new_request =
          create_request(request_pipe_name, response_pipe_name);
      append_request(queue, new_request);

      // Wake up all threads waiting for a Request
      pthread_cond_broadcast(&cond);

      // Unlocks the Queue
      if (pthread_mutex_unlock(&queue->mutex) != 0) {
        fprintf(stderr, "[ERR]: failed unlocking a mutex: %s\n",
                strerror(errno));
        exit(EXIT_FAILURE);
      }
    }
  }
}