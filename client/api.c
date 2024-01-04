#include "api.h"
#include "util.h"  
#include "constants.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int request_pipe;
int response_pipe;
int server_pipe;

int session_id;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  //TODO: create pipes and connect to the server
  
  // Unlink request pipe
  if (unlink(req_pipe_path) != 0 && errno != ENOENT) {
      fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", req_pipe_path,
              strerror(errno));
      exit(EXIT_FAILURE);
  }

  // Create request pipe
  if (mkfifo(req_pipe_path, 0640) != 0) {
      fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }
  
  // Unlink response pipe
  if (unlink(resp_pipe_path) != 0 && errno != ENOENT) {
      fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", resp_pipe_path,
              strerror(errno));
      exit(EXIT_FAILURE);
  }

  // Create response pipe
  if (mkfifo(resp_pipe_path, 0640) != 0) {
      fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }

  // Open server pipe for writing
  // This waits for someone to open it for reading
  server_pipe = open(server_pipe_path, O_WRONLY);
  if (server_pipe == -1) {
      fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  }

  // write to server pipe
  char req_pipe[NAME_LEN];
  memset(req_pipe, 0, NAME_LEN);
  strcpy(req_pipe, req_pipe_path);
  char resp_pipe[NAME_LEN];
  memset(req_pipe, 0, NAME_LEN);
  strcpy(resp_pipe, resp_pipe_path);
  char code = OP_SETUP;
  write(server_pipe, &code, sizeof(char));
  write(server_pipe, req_pipe, NAME_LEN);
  write(server_pipe, resp_pipe, NAME_LEN);

  read(response_pipe, &session_id, sizeof(int));

  // Open request pipe for writing
  // This waits for someone to open it for reading
  request_pipe = open(req_pipe_path, O_WRONLY);
  if (request_pipe == -1) {
      fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  } 

  // Open response pipe for reading
  // This waits for someone to open it for writing
  response_pipe = open(resp_pipe_path, O_WRONLY);
  if (response_pipe == -1) {
      fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
  } 

  return 1;
}

int ems_quit(void) { 
  // Create and send message
  char code = OP_QUIT;
  write(request_pipe, &code, sizeof(char));
  write(request_pipe, &session_id, sizeof(int));
  //TODO: close pipes
  close(request_pipe);
  close(response_pipe);
  return 1;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  char code = OP_CREATE;
  write(request_pipe, &code, sizeof(char));
  write(request_pipe, &session_id, sizeof(int));
  write(request_pipe, &event_id, sizeof(unsigned int));
  write(request_pipe, &num_rows, sizeof(size_t));
  write(request_pipe, &num_cols, sizeof(size_t));
  read(response_pipe, NULL, sizeof(int));
  //TODO: send create request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  char code = OP_RESERVE;
  write(request_pipe, &code, sizeof(char));
  write(request_pipe, &session_id, sizeof(int));
  write(request_pipe, &event_id, sizeof(unsigned int));
  write(request_pipe, &num_seats, sizeof(size_t));
  write(request_pipe, xs, sizeof(size_t)*num_seats);
  write(request_pipe, ys, sizeof(size_t)*num_seats);
  read(response_pipe, NULL, sizeof(int));
  //TODO: send reserve request to the server (through the request pipe) and wait for the response (through the response pipe)
  return 1;
}

int ems_show(int out_fd, unsigned int event_id) {
  char code = OP_SHOW;
  write(request_pipe, &code, sizeof(char));
  write(request_pipe, &session_id, sizeof(int));
  write(request_pipe, &event_id, sizeof(unsigned int));
  read(response_pipe, NULL, sizeof(int));
  size_t num_rows, num_cols;
  read(response_pipe, &num_rows, sizeof(size_t));
  read(response_pipe, &num_cols, sizeof(size_t));
  unsigned int* seats = malloc(num_rows * num_cols * sizeof(unsigned int));
  read(response_pipe, seats, sizeof(unsigned int) * num_cols * num_rows);
  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  //TODO: write output to file
  free(seats);
  return 1;
}

int ems_list_events(int out_fd) {
  char code = OP_LIST_EVENTS;
  write(request_pipe, &code, sizeof(char));
  write(request_pipe, &session_id, sizeof(int));
  read(response_pipe, NULL, sizeof(int));
  size_t num_events;
  read(response_pipe, &num_events, sizeof(size_t));
  unsigned int* ids = malloc(num_events * sizeof(unsigned int));
  read(response_pipe, ids, sizeof(unsigned int) * num_events);
  //TODO: send list request to the server (through the request pipe) and wait for the response (through the response pipe)
  //TODO: write output to file
  return 1;
}
