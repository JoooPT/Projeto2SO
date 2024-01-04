#include "common/util.h"
#include "common/constants.h"
#include "queue.h"
#include <pthread.h>
#include <sys/unistd.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int run_session(struct Request request) {

    // Open request pipe for writing
    // This waits for someone to open it for reading
    int req_pipe = open(request.request_pipe_name, O_RDONLY);
    if (req_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } 

    // Open response pipe for reading
    // This waits for someone to open it for writing
    int resp_pipe = open(request.response_pipe_name, O_WRONLY);
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
            return 
        }
}