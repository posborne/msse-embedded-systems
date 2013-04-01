#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pololu/orangutan.h>
#include "menu.h"
#include "leds.h"
#include "timers.h"
#include "log.h"

typedef struct {
    char ring_buffer[32];
    uint8_t ring_buffer_position;
    char command_buffer[128];
    uint8_t command_buffer_length;
} receive_buffer_t;

// local "global" data structures
static led_state_t * g_led_state;
static timers_state_t * g_timers_state;
static receive_buffer_t g_receive_buffer = {
        .ring_buffer_position = 0,
        .command_buffer_length = 0
};

#define MENU "\rMenu: {TPZ} {RGYA} <int>\r\n"

static int leds_set_toggle(char color, const uint16_t ms) {
    // For each color, if ms is 0, turn it off by changing data direction to input
    // If it is >0, set data direction to output
    uint32_t us = ms * 1000UL;
    if ((color == 'R') || (color == 'A')) {
        if (ms == 0) {
            LED_DISABLE(RED);
        } else {
            LED_ENABLE(RED);
        }
        g_timers_state->red_period = ms;
    }

    if ((color == 'Y') || (color == 'A')) {
        if (ms == 0) {
            LED_DISABLE(YELLOW);
        } else {
            LED_ENABLE(YELLOW);
            timers_setup_timer(TIMER_COUNTER3, TIMER_MODE_CTC, us);
        }
        g_timers_state->yellow_period = ms;
    }


    if ((color == 'G') || (color == 'A')) {
        if (ms == 0) {
            LED_DISABLE(GREEN);
        } else {
            LED_ENABLE(GREEN);
            timers_setup_timer(TIMER_COUNTER1, TIMER_MODE_CTC, us);
        }
        g_timers_state->green_period = ms;
    }
    return 0;
}

static int
menu_process_command(char * command)
{
    int parsed;
    char color;
    char op_char;
    uint16_t value = 0;

    parsed = sscanf(command, "%c %c %u", &op_char, &color, &value);
    if (parsed < 2) {
        LOG("Command \"%s\" not valid.\r\n", command);
        return -1;
    }

    LOG("Parsed as op:%c color:%c value:%d\r\n", op_char, color, value);

    /* convert color to upper and check if valid*/
    color -= 32 * (color >= 'a' && color <= 'z');
    switch (color) {
    case 'R':
    case 'G':
    case 'Y':
    case 'A':
        break;
    default:
        LOG("Bad color\r\n");
        return 0;
    }

    /* Check valid command and implement */
    switch (op_char) {

    /* change toggle frequencey for <color> LED */
    case 'T':
    case 't':
        leds_set_toggle(color, value);
        break;

    /* print counter for <color> LED */
    case 'P':
    case 'p':
        switch (color) {
        case 'R':
            LOG("R toggles: %lu\r\n", g_led_state->red_toggles);
            break;
        case 'G':
            LOG("G toggles: %lu\r\n", g_led_state->green_toggles);
            break;
        case 'Y':
            LOG("Y toggles: %lu\r\n", g_led_state->yellow_toggles);
            break;
        case 'A':
            LOG("toggles R:%lu  G:%lu  Y:%lu\r\n",
                    g_led_state->red_toggles,
                    g_led_state->green_toggles,
                    g_led_state->yellow_toggles);
            break;
        default:
            LOG("How did you get here too??\r\n");
            break;
        }
        break;

    /* zero counter for <color> LED */
    case 'Z':
    case 'z':
        switch (color) {
        case 'R':
            g_led_state->red_toggles = 0;
            break;
        case 'G':
            g_led_state->green_toggles = 0;
            break;
        case 'Y':
            g_led_state->yellow_toggles = 0;
            break;
        case 'A':
            g_led_state->red_toggles = g_led_state->green_toggles = g_led_state->yellow_toggles = 0;
            break;
        default:
            LOG("How did you get here 3??\r\n");
            break;
        }
        break;
    default:
        LOG("%s does not compute??\r\n", g_receive_buffer.ring_buffer);
        break;

    } // end switch(op_char)
    return 0;
}

/*
 * Initiailize the menuing system
 */
int menu_init(led_state_t * led_state, timers_state_t * timers_state)
{
    /* dependencies */
    g_led_state = led_state;
    g_timers_state = timers_state;

    // Set the baud rate to 9600 bits per second.  Each byte takes ten bit
    // times, so you can get at most 960 bytes per second at this speed.
    serial_set_baud_rate(USB_COMM, 9600);
    serial_set_mode(USB_COMM, SERIAL_CHECK);
    serial_receive_ring(
            USB_COMM, g_receive_buffer.ring_buffer,
            sizeof(g_receive_buffer.ring_buffer));
    LOG("USB Serial Initialized\r\n");
    LOG(MENU);
    LOG("#> ");
    return 0;
}

// Parse user-input and implement commands
int menu_service(void)
{
    int received_bytes = 0;
    char c;
    // process received bytes from the ring buffer and move them into the
    // command buffer
    //
    // TODO: possible buffer overflow for really long lines
    while ((received_bytes = serial_get_received_bytes(USB_COMM)) != g_receive_buffer.ring_buffer_position) {
        // add the current byte to the command buffer
        c = g_receive_buffer.ring_buffer[g_receive_buffer.ring_buffer_position];
        LOG("%c", c);
        g_receive_buffer.command_buffer[g_receive_buffer.command_buffer_length++] = c;
        g_receive_buffer.ring_buffer_position =
                (g_receive_buffer.ring_buffer_position == sizeof(g_receive_buffer.ring_buffer) - 1) ? 0 : g_receive_buffer.ring_buffer_position + 1;
    }

    // Scan for a "line" (that is words followed by \r\n or \r\r\n. If found,
    // record that we have a match and null terminate the string
    {
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
            menu_process_command(g_receive_buffer.command_buffer);
            LOG("#> ");
            memcpy(g_receive_buffer.command_buffer,
                   &g_receive_buffer.command_buffer[end_of_command + 1],
                   sizeof(g_receive_buffer.command_buffer) - end_of_command);
            g_receive_buffer.command_buffer_length -= (end_of_command + 1);
        }
    }
    return 0;

} // end menu()

