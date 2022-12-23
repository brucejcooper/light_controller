#ifndef __SCHED_CALLBACK_H__
#define __SCHED_CALLBACK_H__

typedef void (*user_mode_callback_t)(void *args);

void schedule_call(user_mode_callback_t cb, void *args);
void poll_calls();

#endif