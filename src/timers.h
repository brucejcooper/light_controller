#ifndef ___TIMERS_H___
#define ___TIMERS_H___

#include <stdint.h>

typedef void (*timer_handler_t)(void *context);

#define MSEC_TO_TIMER_TICKS(msec) (msec*2048/1000)

#define TIMER_BUTTON1 0
#define TIMER_BUTTON2 1
#define TIMER_BUTTON3 2
#define TIMER_BUTTON4 3
#define TIMER_BUTTON5 4
#define TIMER_REPEAT 5
#define MAX_TIMERS 6

typedef struct timer_t {
    timer_handler_t handler;
    uint16_t timeout;
    struct timer_t *next;
    void *context;
} timer_t;



void setLongTimer(uint16_t seconds, timer_handler_t handler, void *ctx);
void startTimer(uint16_t period, timer_handler_t handler, void *ctx, timer_t *timer);
void cancelTimer(timer_t *timer);

void initialise_timers();



#endif