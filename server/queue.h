#ifndef SERVER_QUEUE_H
#define SERVER_QUEUE_H

#include <pthread.h>
#include <stddef.h>

struct Request {
    char request_pipe_name[40];
    char response_pipe_name[40];
};

struct QueueNode{
    struct Request* request;
    struct QueueNode* next;
};

struct Queue{
    struct QueueNode* head;
    struct QueueNode* tail;
    pthread_mutex_t mutex;
};

/// Creates a new Queue
/// @return Newly created Queue or NULL on failure
struct Queue* create_queue();

/// Appends a request to the end of the Queue
/// @param queue Queue to be modified
/// @param request Request to be appended
/// @return 0 if append was sucessful or 1 otherwise
int append_request(struct Queue* queue, struct Request* request);


/// Pops the first request from the Queue
/// @param queue Queue to be modified
/// @return The head Request fo the Queue or NULL on failure
struct Request* pop_request(struct Queue* queue);

/// Frees all the Requests form the Queue
/// @param queue Queue to be modified
void free_queue(struct Queue* queue);

#endif SERVER_QUEUE_H
