#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pololu/orangutan.h>
#include "log.h"
#include "cli.h"

#define MAX_CLI_COMMANDS (10)
#define UNUSED_PARAMETER(x) (void)(x)

typedef struct {
    char ring_buffer[32];
    uint8_t ring_buffer_position;
    char command_buffer[128];
    uint8_t command_buffer_length;
} receive_buffer_t;

typedef struct {
    cli_command_t commands[MAX_CLI_COMMANDS];
    int number_commands;
} cli_commands_t;

static receive_buffer_t g_receive_buffer = {
        .ring_buffer_position = 0,
        .command_buffer_length = 0
};

static cli_commands_t cli_commands = {
        .commands = {},
        .number_commands = 0
};

static int clicmd_show_commands(char const * const command)
{
    int i;

    UNUSED_PARAMETER(command);

    LOG("Available Commands:\r\n");
    for (i = 0; i < cli_commands.number_commands; i++) {
        cli_command_t cmd = cli_commands.commands[i];
        LOG("%s - %s\r\n", cmd.command, cmd.description);
    }
    return 0;
}

/*
 * Actually process a command that has been received
 *
 * Here, we iterate through all the commands that have been registered with
 * the goal of finding a command that matches.  If no matching command is
 * found, we will log an error.  If we do find one, we will allow the command
 * handler to process the command (everything after the first token.
 */
static void cli_process_command(char const * const command)
{
    int i, root_command_length;
    char * first_space;
    char * root_command;
    char * remaining_arguments = NULL;
    cli_command_t * matching_command = NULL;

    /* determine first command based on first space location */
    first_space = strchr(command, ' ');
    if (first_space == NULL) {
        root_command = (char *)command;
        root_command_length = strlen(command);
    } else {
        root_command = (char *)command;
        root_command_length = (size_t)(first_space - command);
        remaining_arguments = (first_space + 1);
    }

    /* do a linear search on our commands for a match */
    for (i = 0; i < cli_commands.number_commands; i++) {
        cli_command_t * command = &cli_commands.commands[i];
        if (root_command_length && strncmp(command->command, root_command, root_command_length) == 0) {
            matching_command = command;
            break;
        }
    }

    /* handle command or print error message */
    if (matching_command != NULL) {
        matching_command->handler(remaining_arguments);
    } else {
        LOG("Unknown Command \"%s\"\r\n", command);
    }
}

/*
 * Initiailize the menuing system
 */
int cli_init(void)
{
    // Set the baud rate to 9600 bits per second.  Each byte takes ten bit
    // times, so you can get at most 960 bytes per second at this speed.
    serial_set_baud_rate(USB_COMM, 9600);
    serial_set_mode(USB_COMM, SERIAL_CHECK);
    serial_receive_ring(
            USB_COMM, g_receive_buffer.ring_buffer,
            sizeof(g_receive_buffer.ring_buffer));
    LOG("USB Serial Initialized\r\n");
    LOG("#> ");
    CLI_REGISTER("?", "Show Commands", clicmd_show_commands);
    return 0;
}

/*
 * Register a CLI command
 */
void cli_register(cli_command_t command)
{
    cli_commands.commands[cli_commands.number_commands] = command;
    cli_commands.number_commands++;
}

// Parse user-input and implement commands
int cli_service(void)
{
    int received_bytes = 0;
    char c;
    bool buffers_updated = false;
    // process received bytes from the ring buffer and move them into the
    // command buffer
    while ((received_bytes = serial_get_received_bytes(USB_COMM)) != g_receive_buffer.ring_buffer_position) {
        // add the current byte to the command buffer
        buffers_updated = true;
        c = g_receive_buffer.ring_buffer[g_receive_buffer.ring_buffer_position];
        LOG("%c", c);
        if (c == '\b') {
            if (g_receive_buffer.command_buffer_length) {
                LOG(" \b"); /* erase previous character from screen */
                g_receive_buffer.command_buffer_length--; /* backspace */
            } else {
                LOG(" "); /* fill back space */
            }
        } else {
            g_receive_buffer.command_buffer[g_receive_buffer.command_buffer_length++] = c;
        }
        if (g_receive_buffer.ring_buffer_position == sizeof(g_receive_buffer.ring_buffer)) {
            g_receive_buffer.ring_buffer_position = 0;
        } else {
            g_receive_buffer.ring_buffer_position++;
        }
    }

    // Scan for a "line" (that is words followed by \r\n or \r\r\n. If found,
    // record that we have a match and null terminate the string
    if (buffers_updated) {
        char *loc;
        int idx;
        int end_of_command = false;
        bool command_waiting = false;
        if ((loc = strstr(g_receive_buffer.command_buffer, "\r\n")) != NULL) {
            LOG("\r\n");
            idx = loc - g_receive_buffer.command_buffer;
            command_waiting = true;
            end_of_command = idx + 1;
            g_receive_buffer.command_buffer[idx] = '\0';
            g_receive_buffer.command_buffer[idx + 1] = '\0';
        } else if ((loc = strchr(g_receive_buffer.command_buffer, '\r')) != NULL) {
            LOG("\r\n");
            idx = loc - g_receive_buffer.command_buffer;
            command_waiting = true;
            end_of_command = idx;
            g_receive_buffer.command_buffer[idx] = '\0';
        }

        if (command_waiting) {
            // now, process the command and move any modify the contents of the
            // command_buffer to be in a valid state again
            cli_process_command(g_receive_buffer.command_buffer);
            LOG("#> ");
            memcpy(g_receive_buffer.command_buffer,
                   &g_receive_buffer.command_buffer[end_of_command + 1],
                   sizeof(g_receive_buffer.command_buffer) - end_of_command);
            g_receive_buffer.command_buffer_length -= (end_of_command + 1);
        }
    }
    return 0;

} // end menu()

