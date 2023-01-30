#ifndef __BUTTONS_H__
#define __BUTTONS_H__


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


extern void buttons_init();
bool poll_buttons();

#endif