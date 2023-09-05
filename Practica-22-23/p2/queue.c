#include <stdlib.h>
#include <threads.h>

// circular array
typedef struct _queue {
    int size;
    int used;
    int first;
    void **data;
    mtx_t mutex;
} _queue;

#include "queue.h"

queue q_create(int size) {
    queue q = malloc(sizeof(_queue));

    q->size  = size;
    q->used  = 0;
    q->first = 0;
    q->data  = malloc(size * sizeof(void *));
    mtx_init(q->mutex, 0);

    return q;
}

int q_elements(queue q) {
    
    mtx_lock(&q->mutex);
    int used = q-> used;
    mtx_unlock(&q->mutex);
    
    return q->used;
}

int q_insert(queue q, void *elem) {
    
    mtx_lock(&q->mutex);
    if(q->size == q->used){
        cnd_wait(&q->mutex);
    }

    q->data[(q->first + q->used) % q->size] = elem;
    q->used++;
    mtx_unlock(&q->mutex);

    return 0;
}

void *q_remove(queue q) {
    
    mtx_lock(&q->mutex);
    while(q->used == 0){
        cnd_wait(..., &q->mutex);
    }
    res = q->data[q->first];

    q->first = (q->first + 1) % q->size;
    q->used--;
    mtx_unlock(&q->mutex);

    return res;
}

void q_destroy(queue q) {
    mtx_destroy(&q->mutex);
    free(q->data);
    free(q);
}