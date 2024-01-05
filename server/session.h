#ifndef SERVER_SESSION_H
#define SERVER_SESSION_H

#include <pthread.h>

#define END_THREAD 69 // code to terminate the thread

struct Arguments {
  struct Queue *queue;
  pthread_cond_t *cond;
  int session_id;
};

/// Each thread fetches a request from the queue and creates a session with a
/// client
/// @param args struct Arguments with a queue, condition variable and a
/// session_id
void *run_thread(void *args);

#endif