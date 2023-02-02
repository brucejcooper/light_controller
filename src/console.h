
#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <stdio.h>

void console_init();
void console_flush();

void log_uint24(char *str, uint32_t val);
void log_uint16(char *str, uint16_t val);
void log_uint8(char *str, uint8_t val);
void log_info(char *str);
void log_char(char character);
void log_hex(uint8_t val);

#endif