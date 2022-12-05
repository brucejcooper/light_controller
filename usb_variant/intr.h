#ifndef ___ATINTR_H___
#define ___ATINTR_H___


typedef void (*isr_handler_t)();
typedef void (*isr_pulse_handler_t)(uint16_t);


void set_isrs(isr_handler_t tca0_cmp, isr_handler_t tca0_ovf, isr_pulse_handler_t tcb0);
void set_input_pulse_handler(isr_pulse_handler_t tcb0);
void set_output_pulse_handler(isr_handler_t h);


#endif