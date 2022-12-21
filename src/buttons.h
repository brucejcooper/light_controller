#ifndef __BUTTONS_H__
#define __BUTTONS_H__

typedef enum {
  EVENT_NONE,
  EVENT_PRESSED,
  EVENT_LONG_PRESSED,
  EVENT_RELEASED,
} button_event_t;

#define NUM_BUTTONS (1)

extern void buttons_init();
void buttons_set_wake_from_sleep_enabled(bool enabled);

#endif