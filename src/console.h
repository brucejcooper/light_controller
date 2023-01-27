
#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <stdio.h>


typedef enum {
    CMD_OK,                // Everything went right
    CMD_NO_OP,             // Nothing to do
    CMD_DEFERRED_RESPONSE, // Indicates that we don't have an answer immediately, but something else will re-call sendCommandResponse at a later time.
    CMD_BAD_INPUT,         // User entered bad input
    CMD_FAIL,              // We couldn't execute the command
    CMD_BUSY,              // The processor is already busy doing something else.  Retry once that command has completed.
    CMD_FULL,              // Some resource is exhausted (Input pulse buffer is full)
} command_response_t;


typedef command_response_t (*command_hanlder_t)(char *cmd);

void console_init(command_hanlder_t handler);

void log_uint24(char *str, uint32_t val);
void log_uint16(char *str, uint16_t val);
void log_uint8(char *str, uint8_t val);
void log_info(char *str);
void log_char(char character);

void sendCommandResponse(command_response_t response);
void printPrompt();
#endif