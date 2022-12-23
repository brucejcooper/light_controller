#ifndef ___ATINTR_H___
#define ___ATINTR_H___


typedef void (*isr_handler_t)();


void startSingleShotTimer(uint16_t to, isr_handler_t tca0_ovf);
void clearTimeout();

#endif