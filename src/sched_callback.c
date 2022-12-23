
#include "sched_callback.h"
#include <stdint.h>
#include <string.h>
#include <util/atomic.h>
#include <stdlib.h>

typedef struct {
    user_mode_callback_t cb;
    void *args;
} queueentry_t;

#define QUEUEDEPTH 5

static queueentry_t queue[QUEUEDEPTH];
static volatile uint8_t queueDepth;

void schedule_call(user_mode_callback_t cb, void *args) {
    cli();
    if (cb && queueDepth < QUEUEDEPTH) {
        queue[queueDepth].cb = cb;
        queue[queueDepth++].args = args;
    } else if (args != NULL) {
        free(args);
    }
    sei();
}

void poll_calls() {
    if (queueDepth) {
        cli();
        user_mode_callback_t cb = queue[0].cb;
        void *args = queue[0].args;
        if (--queueDepth) {
            memcpy(queue, queue+1, queueDepth*sizeof(queueentry_t));
        }
        sei();
        cb(args);
        if (args) {
            free(args);
        }
    }
}

