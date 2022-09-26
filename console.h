
#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <stdio.h>

typedef void (*command_hanlder_t)(char *cmd);

void console_init(command_hanlder_t handler);
void USART0_sendChar(char c);
void log_info(char *str, ...);

#endif