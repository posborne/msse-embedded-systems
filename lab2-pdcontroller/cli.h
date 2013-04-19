/*
 * Header file for CLI stuff
 */
#ifndef __CLI_H
#define __CLI_H

#include <inttypes.h>
#include "macros.h"

typedef int (*cli_command_handler_t)(char const * const args);

typedef struct {
    char * command;
    char * description;
    cli_command_handler_t handler;
} cli_command_t;

int cli_init(void);
int cli_service(void);
void cli_register(cli_command_t command);

#define CLI_REGISTER(...) do { \
    int i; \
    static cli_command_t _cmds_[] = {__VA_ARGS__}; \
    for (i = 0; i < COUNT_OF(_cmds_);  i++) \
        cli_register(_cmds_[i]); \
    } while (0)

#endif //__CLI_H
