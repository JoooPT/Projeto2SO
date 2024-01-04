#include <pthread.h>
#include <sys/unistd.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "common/util.h"
#include "common/constants.h"
#include "operations.h"
#include "queue.h"
#include "session.h"

void *run_thread(void *args){
    struct Arguments* arguments = (struct Arguments*)args;

    while(1){
        //Locks the queue to retrieve a request
        if(pthread_mutex_lock(&arguments->queue->mutex) != 0){
            fprintf(stderr, "[ERR]: failed locking a muted: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        //If the queue is empty waits for a signal that a request was made
        while(arguments->queue->head == NULL){
            pthread_cond_wait(&arguments->cond, &arguments->queue->mutex);
        }

        //Fetchs a Request from the Queue
        struct Request* new_request = pop_request(arguments->queue);

        //Unlocks the Queue 
        if(pthread_mutex_unlock(&arguments->queue->mutex)!= 0){
            fprintf(stderr, "[ERR]: failed unlocking a muted: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        // Starts the Session with the new client
        if(new_request != NULL){
            if(run_session(new_request) == END_THREAD){
                break;
            }
        }
    }
}


int run_session(struct Request* request) {

    // Open request pipe for writing
    // This waits for someone to open it for reading
    int req_pipe = open(request->request_pipe_name, O_RDONLY);
    if (req_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } 

    // Open response pipe for reading
    // This waits for someone to open it for writing
    int resp_pipe = open(request->response_pipe_name, O_WRONLY);
    if (resp_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } 

    char code;
    int session_id;

    while (1)
        get_msg(req_pipe, &code, sizeof(char));
        get_msg(req_pipe, &session_id, sizeof(int));
        
        switch(code) {
        case OP_QUIT:
            // Unlink response pipe
            if (unlink(request->response_pipe_name) != 0 && errno != ENOENT) {
                fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", request->response_pipe_name,
                        strerror(errno));
                return(1);
            }
            // Unlink request pipe
            if (unlink(request->request_pipe_name) != 0 && errno != ENOENT) {
                fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", request->request_pipe_name,
                        strerror(errno));
                return(1);
            }
            return 0;
        
        case OP_CREATE:
            unsigned int event_id;
            size_t num_rows, num_cols;
            get_msg(req_pipe, &event_id, sizeof(unsigned int));
            get_msg(req_pipe, &num_rows, sizeof(size_t));
            get_msg(req_pipe, &num_cols, sizeof(size_t));
            int ret = ems_create(event_id, num_rows, num_cols);
            send_msg(resp_pipe, &ret, sizeof(int));
            
        case OP_RESERVE:
            unsigned int event_id;
            size_t num_seats;
            size_t* xs = malloc(sizeof(size_t)*num_seats);
            size_t* ys = malloc(sizeof(size_t)*num_seats);
            get_msg(req_pipe, &event_id, sizeof(unsigned int));
            get_msg(req_pipe, &num_seats, sizeof(size_t));
            get_msg(req_pipe, xs, sizeof(size_t)*num_seats);
            get_msg(req_pipe, ys, sizeof(size_t)*num_seats);
            int ret = ems_reserve(event_id, num_seats, xs, ys);
            send_msg(resp_pipe, &ret, sizeof(int));
        
        case OP_SHOW:
            unsigned int event_id;
            get_msg(req_pipe, &event_id, sizeof(unsigned int));
            ems_show(resp_pipe, event_id);

        case OP_LIST_EVENTS:
            
        }

}