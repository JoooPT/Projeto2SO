#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

void send_msg(int pipe, void *src, size_t bytes) {
  if (write(pipe, src, bytes) < 0) {
    fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void get_msg(int pipe, void *dest, size_t bytes) {
  if (read(pipe, dest, bytes) < 0) {
    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}