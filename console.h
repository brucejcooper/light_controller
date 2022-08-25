
#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <stdio.h>

void console_init();
void USART0_sendChar(char c);
void log_info(char *str, ...);

#endif