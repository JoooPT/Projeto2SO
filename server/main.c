#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#include "queue.h"

int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }

  char* endptr;
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

  //TODO: Intialize server, create worker threads

  char* fifo_pathname = argv[1];

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
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  pthread_t *threads = malloc((unsigned long)MAX_SESSION_COUNT * sizeof(pthread_t));

  struct Queue* queue = create_queue();

  while (1) {
    //TODO: Read from pipe
    char code;
    if (read(server_pipe, &code, sizeof(char)) < 0) {
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
    if (code == OP_SETUP) {
      //pthread_create(&threads[i], NULL, run_thread, (void *)&args_list[i]);
    }
    //TODO: Write new client to the producer-consumer buffer
  }

  //TODO: Close Server

  ems_terminate();
}