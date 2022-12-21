#ifndef ___ATINTR_H___
#define ___ATINTR_H___


typedef void (*isr_handler_t)();
typedef void (*isr_pulse_handler_t)(uint16_t);


void set_incoming_pulse_handler(isr_pulse_handler_t tcb0);
void set_input_pulse_handler(isr_pulse_handler_t tcb0);
void startSingleShotTimer(uint16_t to, isr_handler_t tca0_ovf);
void clearTimeout();

#endif