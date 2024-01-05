#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

int send_msg(int pipe, void *src, size_t bytes) {
  if (write(pipe, src, bytes) < 0) {
    fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

int get_msg(int pipe, void *dest, size_t bytes) {
  if (read(pipe, dest, bytes) < 0) {
    fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}