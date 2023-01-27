#ifndef __BUTTONS_H__
#define __BUTTONS_H__

#include "sched_callback.h"

typedef enum {
  EVENT_NONE,
  EVENT_PRESSED,
  EVENT_LONG_PRESSED,
  EVENT_RELEASED,
} button_event_type_t;

#define NUM_BUTTONS (1)

typedef struct {
  button_event_type_t type;
  uint8_t index;
} button_event_t;


extern void buttons_init(user_mode_callback_t cb);
void buttons_set_wake_from_sleep_enabled(bool enabled);

bool poll_for_events(button_event_t *evt);
#endif