#ifndef __MYQUEUE_H__
#define __MYQUEUE_H__

#include <stdint.h>

typedef struct {
    uint8_t size;
    uint8_t head;
    uint8_t tail;
    uint8_t *entries;
} queue_t;

typedef enum {
    QUEUE_OK = 0,
    QUEUE_FULL,
    QUEUE_EMPTY,
    QUEUE_NOMEM,
} queue_result_t;

extern queue_result_t queue_init(queue_t *q, uint8_t sz);
extern queue_result_t queue_push(queue_t *q, uint8_t event);
extern queue_result_t queue_pop(queue_t *q, uint8_t *out);


#endif