#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "api.h"
#include "common/constants.h"
#include "common/util.h"

int request_pipe;
int response_pipe;
int server_pipe;

int session_id;

int ems_setup(char const *req_pipe_path, char const *resp_pipe_path,
              char const *server_pipe_path) {
  // TODO: create pipes and connect to the server

  // Unlink request pipe
  if (unlink(req_pipe_path) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", req_pipe_path,
            strerror(errno));
    return 1;
  }

  // Create request pipe
  if (mkfifo(req_pipe_path, 0640) != 0) {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    return 1;
  }

  // Unlink response pipe
  if (unlink(resp_pipe_path) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", resp_pipe_path,
            strerror(errno));
    return 1;
  }

  // Create response pipe
  if (mkfifo(resp_pipe_path, 0640) != 0) {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    return 1;
  }

  // Open server pipe for writing
  // This waits for someone to open it for reading
  server_pipe = open(server_pipe_path, O_WRONLY);
  if (server_pipe == -1) {
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    return 1;
  }

  // write to server pipe
  char req_pipe[NAME_LEN];
  memset(req_pipe, 0, NAME_LEN * sizeof(char));
  strcpy(req_pipe, req_pipe_path);
  char resp_pipe[NAME_LEN];
  memset(resp_pipe, 0, NAME_LEN * sizeof(char));
  strcpy(resp_pipe, resp_pipe_path);
  char code = OP_SETUP;
  send_msg(server_pipe, &code, sizeof(char));
  send_msg(server_pipe, req_pipe, NAME_LEN);
  send_msg(server_pipe, resp_pipe, NAME_LEN);

  // Open request pipe for writing
  // This waits for someone to open it for reading
  request_pipe = open(req_pipe_path, O_WRONLY);
  if (request_pipe == -1) {
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    return 1;
  }

  // Open response pipe for reading
  // This waits for someone to open it for writing
  response_pipe = open(resp_pipe_path, O_RDONLY);
  if (response_pipe == -1) {
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    return 1;
  }

  get_msg(response_pipe, &session_id, sizeof(int));

  return 0;
}

int ems_quit(void) {
  // Create and send message
  char code = OP_QUIT;
  send_msg(request_pipe, &code, sizeof(char));
  send_msg(request_pipe, &session_id, sizeof(int));
  // TODO: close pipes
  close(request_pipe);
  close(response_pipe);
  return 1;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  int exit;
  char code = OP_CREATE;
  send_msg(request_pipe, &code, sizeof(char));
  send_msg(request_pipe, &session_id, sizeof(int));
  send_msg(request_pipe, &event_id, sizeof(unsigned int));
  send_msg(request_pipe, &num_rows, sizeof(size_t));
  send_msg(request_pipe, &num_cols, sizeof(size_t));
  get_msg(response_pipe, &exit, sizeof(int));
  // TODO: send create request to the server (through the request pipe) and wait
  // for the response (through the response pipe)
  return exit;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t *xs,
                size_t *ys) {
  int exit;
  char code = OP_RESERVE;
  send_msg(request_pipe, &code, sizeof(char));
  send_msg(request_pipe, &session_id, sizeof(int));
  send_msg(request_pipe, &event_id, sizeof(unsigned int));
  send_msg(request_pipe, &num_seats, sizeof(size_t));
  send_msg(request_pipe, xs, sizeof(size_t) * num_seats);
  send_msg(request_pipe, ys, sizeof(size_t) * num_seats);
  get_msg(response_pipe, &exit, sizeof(int));
  // TODO: send reserve request to the server (through the request pipe) and
  // wait for the response (through the response pipe)
  return exit;
}

int ems_show(int out_fd, unsigned int event_id) {
  int exit;
  char code = OP_SHOW;
  send_msg(request_pipe, &code, sizeof(char));
  send_msg(request_pipe, &session_id, sizeof(int));
  send_msg(request_pipe, &event_id, sizeof(unsigned int));
  get_msg(response_pipe, &exit, sizeof(int));
  if (exit == 0) {
    size_t num_rows, num_cols;
    get_msg(response_pipe, &num_rows, sizeof(size_t));
    get_msg(response_pipe, &num_cols, sizeof(size_t));
    unsigned int *seats = malloc(num_rows * num_cols * sizeof(unsigned int));
    get_msg(response_pipe, seats, sizeof(unsigned int) * num_cols * num_rows);
    // TODO: send show request to the server (through the request pipe) and wait
    // for the response (through the response pipe)
    // TODO: write output to file
    int i = 0;
    char newLine = '\n';
    for (size_t row = 0; row < num_rows; row++) {
      for (size_t col = 0; col < num_cols; col++) {
        char seat[12];
        snprintf(seat, sizeof(seat), "%u ", seats[i++]);
        write(out_fd, seat, strlen(seat));
      }
      write(out_fd, &newLine, sizeof(char));
    }
    free(seats);
  }
  return exit;
}

int ems_list_events(int out_fd) {
  int exit;
  char code = OP_LIST_EVENTS;
  send_msg(request_pipe, &code, sizeof(char));
  send_msg(request_pipe, &session_id, sizeof(int));
  get_msg(response_pipe, &exit, sizeof(int));
  if (exit == 0) {
    size_t num_events;
    get_msg(response_pipe, &num_events, sizeof(size_t));
    unsigned int *ids = malloc(num_events * sizeof(unsigned int));
    get_msg(response_pipe, ids, sizeof(unsigned int) * num_events);
    for (size_t i = 0; i < num_events; i++) {
      char event[19];
      snprintf(event, sizeof(event), "Event: %u\n", ids[i]);
      write(out_fd, event, strlen(event));
    }
    free(ids);
  }
  else if (exit == 2) {
    char no_events[11] = "No Events\n";
    write(out_fd, no_events, strlen(no_events));
  }
  // TODO: send list request to the server (through the request pipe) and wait
  // for the response (through the response pipe)
  // TODO: write output to file
  return exit;
}
