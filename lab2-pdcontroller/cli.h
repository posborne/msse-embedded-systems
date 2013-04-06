/*******************************************
*
* Header file for menu stuff.
*
*******************************************/
#ifndef __MENU_H
#define __MENU_H

#include <inttypes.h> //gives us uintX_t

#define COUNT_OF(x)   ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
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
    static cli_command_t _cmd_ = {__VA_ARGS__}; \
    cli_register(_cmd_); } while (0)

#endif //__MENU_H
