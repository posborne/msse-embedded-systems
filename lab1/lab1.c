/*
 * SENG5831: Lab Assignment 1
 * --------------------------
 * You will write a program to blink the three separate LEDs at various
 * user-defined frequencies using 4 different methods. A toggle counter
 * will be used to keep track of the number of toggles for each color.
 *
 * Using WCET static or dynamic analysis, determine the number of
 * iterations required in a for-loop to occupy the CPU for 10ms. Use this
 * loop to blink the red LED at 1HZ (i.e. a period of 1000ms).
 *
 * Create a software timer (8-bit) with 1ms resolution, then blink the
 * red LED inside a cyclic executive at a user-specified rate using your
 * software timer. Essentially, the ISR is releasing the red LED task.
 *
 * Create another software timer (timer3) with 100ms resolution (10Hz),
 * and blink the yellow LED inside the ISR for the timer interrupt. In
 * this case, the system is being polled at a specific frequency to
 * determine the readiness of a task.
 *
 * Create a Compare Match Interrupt with a frequency equal to the
 * user-specified frequency for blinking the green LED (use
 * timer1). Generate a PWM pulse on OC1A (aka Port D, pin 5) to toggle
 * green.
 *
 * The LEDs should be connected to the following port pins:
 *
 * Green : Port D, pin 5. Look on the bottom of the board for the SPWM
 * port.  Yellow: Port A, pin 0. This could be any pin, but chose this
 * for consistency across all your projects.  Red: Port A, pin 2. This
 * could be any pin, but chose this for consistency.
 *
 * The above will be controlled with a simple serial-based user
 * interface. The communication between the PC and the microcontroller is
 * handled using interrupts and buffers. The buffer is polled inside of
 * the cyclic executive to check for input. This is an event-triggered
 * task, unlike the blinking of LEDs, which are time-triggered (although
 * you could argue that timer interrupts are events). This code has been
 * provided for you. The menu options are as follows:
 *
 * {Z/z} <color>: Zero the counter for LED <color>
 *
 * {P/p} <color>: Print the coutner for LED <color>
 *
 * {T/t} <color> <int> : Toggle LED <color> every <int> ms.
 *
 *
 * <int> = {0, 100, 200, ... }
 *
 * <color> = {R, r, G, g, Y, y, A, a}
 *
 *
 * Examples:
 *
 * t R 250 : the red LED should toggle at a frequency of 4Hz.
 *
 * Ta 2000 : all LEDs should toggle at a frequency of .5Hz.
 *
 * t Y 0 : this turns the yellow LED off
 *
 * Zr : zero the toggle counter for the red LED
 *
 * Hardware Configuration
 * ----------------------
 * As far as external hardware goes, the following outputs will be used:
 * - Yellow Led: IO_D0
 * - Red Led: On Board, IO_D1
 * - Red Led: On Board, 
 *
 * License
 * -------
 * Copyright (c) 2013 Paul Osborne <osbpau@gmail.com>
 *
 * This file is licensed under the MIT License as specified in
 * LICENSE.txt
 */
#include <pololu/orangutan.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "leds.h"
#include "timers.h"
#include "log.h"
#include "scheduler.h"
#include "menu.h"

#define COUNT_OF(x)   ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

static led_state_t g_led_state;
static timers_state_t g_timers_state;

/*
 * TASK: Logs Service
 *
 * Periodically tell the serial port to handle input/output
 */
static void
service_logs(void)
{
    serial_check();
}

/*
 * TASK: Print Info
 *
 * Periodically print out information about system state to serial port
 */
static void
print_info(void)
{
    LOG(LVL_DEBUG, "ms_ticks     (TC0): %lu\r\n", g_timers_state.ms_ticks);
    LOG(LVL_DEBUG, "yellow_ticks (TC1): %u\r\n", g_led_state.yellow_toggles);
    LOG(LVL_DEBUG, "green_ticks  (TC3): %u\r\n", g_led_state.green_toggles);
}

/*
 * TASK: Update LCD
 *
 * Periodically update the LCD with information about system state
 */
static void
update_lcd(void)
{
    char buf[128];
    clear();
    sprintf(buf, "ticks: %lu", g_timers_state.ms_ticks);
    print(buf);
}

static void
service_menu(void)
{
    menu_service();
}

static task_t g_tasks[] = {
    {"Serial Check", 1/* ms */, service_logs},
    {"Print Info", 60000 /* ms */, print_info},
    {"Update LCD", 250 /* ms */, update_lcd},
    {"Service Menu", 50 /* ms */, service_menu},
};


/*
 * Main Loop
 */
int main() {
    LOG(LVL_DEBUG, "--------------------------------\r\n");
    log_init();
    timers_init(&g_timers_state);
	log_service();
	leds_init(&g_led_state);
	log_service();
	scheduler_init(&g_timers_state, g_tasks, COUNT_OF(g_tasks));
    menu_init(&g_led_state, &g_timers_state);
	g_timers_state.ms_ticks = 0;
	while (1) {
	    // NOTE: this could be handled using the scheduler.  Keeping as is
	    // in order to match assignment requirements more closely
	    if (g_timers_state.release_red) {
            LED_TOGGLE(RED);
            g_led_state.red_toggles++;
            g_timers_state.release_red = false;
	    }

	    // This is where most stuff will happen
	    scheduler_service();
	}
}
