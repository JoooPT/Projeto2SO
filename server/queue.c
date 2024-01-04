#include "queue.h"

#include <pthread.h>
#include <stdlib.h>

struct Queue* create_queue(){
    struct Queue* queue = (struct Queue*)malloc(sizeof(struct Queue));
    if(!queue) return NULL;
    if(pthread_mutex_init(&queue->mutex, NULL)!= 0){
        free(queue);
        return NULL;
    }
    queue->head = NULL;
    queue->tail = NULL;
    return queue;
}

int append_request(struct Queue* queue, struct Request* request){
    if(!queue) return 1;

    struct QueueNode* new_node = (struct QueueNode*)malloc(sizeof(struct QueueNode));
    if(!new_node) return 1;

    new_node->request = request;
    new_node->next = NULL;

    if(queue->head == NULL){
        queue->head = new_node;
        queue->tail = new_node;
    } else {
        queue->tail->next = new_node;
        queue->tail = new_node;
    }

    return 0;
}

struct Request* pop_request(struct Queue* queue){
    if(!queue) return;

    struct QueueNode* temp = queue->head;
    if(!temp) return 1;
    temp = temp->next;

    struct Request* request 

}

void free_queue(struct Queue* queue){
    if(!queue) return;

    struct QueueNode* current = queue->head;
    while(current) {
        struct QueueNode* temp = current;
        current = current->next;
        if(temp->request != NULL){
            free(temp->request);
        }
        free(temp);
    }

    free(queue);

}
