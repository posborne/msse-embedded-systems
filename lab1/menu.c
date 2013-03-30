#include <stdio.h>
#include <stdint.h>
#include <pololu/orangutan.h>
#include "menu.h"
#include "leds.h"
#include "timers.h"
#include "log.h"

typedef struct {
    char buffer[32];
    uint8_t len;
} receive_buffer_t;

// local "global" data structures
static led_state_t * g_led_state;
static led_state_t * g_timers_state;
static receive_buffer_t g_receive_buffer = { .len = 0 };

#define MENU "\rMenu: {TPZ} {RGYA} <int>: "

/*
 * Initiailize the menuing system
 */
int menu_init(led_state_t * led_state, timers_state_t * timers_state)
{
    /* dependencies */
    g_led_state = led_state;
    g_timers_state = timers_state;

    serial_receive_ring(USB_COMM, g_receive_buffer, sizeof(g_receive_buffer.len));
    LOG(LVL_INFO, "USB Serial Initialized\n");
    LOG(LVL_INFO, MENU);
    return 0;
}

// Parse user-input and implement commands
int menu_service(void)
{
    char color;
    char op_char;
    uint8_t* input;
    int parsed;
    int value;

    input = serial_receive(USB_COMM,
            &g_receive_buffer.buffer[g_receive_buffer.len],
            sizeof g_receive_buffer.buffer - g_receive_buffer.len);

    parsed = sscanf(input, "%s %s %d", &op_char, &color, &value);
    printf("Parsed as op:%c color:%c value:%d\n", op_char, color, value);

    /* convert color to upper and check if valid*/
    color -= 32 * (color >= 'a' && color <= 'z');
    switch (color) {
    case 'R':
    case 'G':
    case 'Y':
    case 'A':
        break;
    default:
        printf("Bad color\n");
        return 0;
    }

    /* Check valid command and implement */
    switch (op_char) {

    /* change toggle frequencey for <color> LED */
    case 'T':
    case 't':
        set_toggle(color, value);
        break;

        /* print counter for <color> LED */
    case 'P':
    case 'p':
        switch (color) {
        case 'R':
            printf("R toggles: %d\n", G_red_toggles);
            break;
        case 'G':
            printf("G toggles: %d\n", G_green_toggles);
            break;
        case 'Y':
            printf("Y toggles: %d\n", G_yellow_toggles);
            break;
        case 'A':
            printf(
                    "toggles R:%d  G:%d  Y:%d\n", (int) G_red_toggles, (int) G_green_toggles, (int) G_yellow_toggles);
            break;
        default:
            printf("How did you get here too??\n");
            break;
        }
        break;

        /* zero counter for <color> LED */
    case 'Z':
    case 'z':
        switch (color) {
        case 'R':
            G_red_toggles = 0;
            break;
        case 'G':
            G_green_toggles = 0;
            break;
        case 'Y':
            G_yellow_toggles = 0;
            break;
        case 'A':
            G_red_toggles = G_green_toggles = G_yellow_toggles = 0;
            break;
        default:
            printf("How did you get here 3??\n");
            break;
        }
        break;
    default:
        printf("%s does not compute??\n", input);
        break;

    } // end switch(op_char)
    printf("> ");
    return 0;
} // end menu()

