#include <stdlib.h>
#include <threads.h>
#include <stdbool.h>

// circular array
typedef struct _queue {
    int size;
    int used;
    int first;
    void **data;
    cnd_t full_queue;
    cnd_t empty_queue;
    mtx_t mutex;
    bool ended;
} _queue;

#include "queue.h"

/*
 * Function: end
 * ----------------------
 * It is used to indicate 
 * when the queue has finished 
 * inserting all the elements 
 * by setting the bool to true.
 *
 * @return void.
 */
void end(queue q){      
    mtx_lock(&q->mutex);
    q->ended = true;
    cnd_broadcast(&q->empty_queue);
    mtx_unlock(&q->mutex);
}

queue q_create(int size) {
    queue q = malloc(sizeof(_queue));

    q->size  = size;
    q->used  = 0;
    q->first = 0;
    q->data  = malloc(size * sizeof(void *));
    mtx_init(&q->mutex, 0);
    cnd_init(&q->full_queue);
    cnd_init(&q->empty_queue);
    q->ended = false;

    return q;
}

int q_elements(queue q) {
    return q->used;     //yo diria de bloquear este tambien
}

int q_insert(queue q, void *elem) {
    
    mtx_lock(&q->mutex);
    while(q->size == q->used) //cola llena
        cnd_wait(&q->full_queue, &q->mutex); //espera a que haya un hueco

    q->data[(q->first + q->used) % q->size] = elem;
    q->used++;
    if (q->used == 1)
        cnd_broadcast(&q->empty_queue);
    mtx_unlock(&q->mutex);
    return 0;
}

void* q_remove(queue q) { 
    void *res;
    
    mtx_lock(&q->mutex);
    while(q->used == 0 && !q->ended) { /*vacío. Esperamos*/
        cnd_wait(&q->empty_queue, &q->mutex);
    }
        
    if (q->used == 0 && q->ended){ //Cola vacía porque hemos terminado. Devuelvo NULL
        mtx_unlock(&q->mutex);
        return NULL;
    }   
        
    res = q->data[q->first];

    q->first = (q->first + 1) % q->size;
    q->used--;

    if (q->used == q->size -1)
        cnd_broadcast(&q->full_queue);
    
    mtx_unlock(&q->mutex);
    return res;
}

void q_destroy(queue q) {
    mtx_destroy(&q->mutex);
    cnd_destroy(&q->full_queue);
    cnd_destroy(&q->empty_queue);
    free(q->data);
    free(q);
}
