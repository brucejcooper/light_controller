#include <stdlib.h>
#include "queue.h"


queue_result_t queue_init(queue_t *q, uint8_t sz) {
    q->size = sz;
    q->head = 0;
    q->tail = 0;
    q->entries = malloc(sz * sizeof(uint8_t));
    if (!q->entries) {
        return QUEUE_NOMEM;
    }
    return QUEUE_OK;
}


queue_result_t queue_push(queue_t *q, uint8_t event) {
    if ((q->tail+1) % q->size == q->head) {
        return QUEUE_FULL;
    }
    q->entries[q->tail] = event;
    q->tail = (q->tail + 1) % q->size;
    return QUEUE_OK;
}

queue_result_t queue_pop(queue_t *q,uint8_t *out) {
    if (q->tail == q->head) {
        return QUEUE_EMPTY;
    }
    *out = q->entries[q->head];
    q->head = (q->head + 1) % q->size;
    return QUEUE_OK;
}