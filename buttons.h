#ifndef __BUTTONS_H__
#define __BUTTONS_H__

#include "queue.h"

typedef enum {
  EVENT_TOGGLE,
  EVENT_DIMMER_BRIGHTEN,
  EVENT_DIMMER_DIM,
} button_event_t;

extern void buttons_init(queue_t *q);


#endif