/*
 * SENG5831: Lab Assignment 2
 */
#include <pololu/orangutan.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "timers.h"
#include "log.h"
#include "scheduler.h"

#define COUNT_OF(x)   ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

static timers_state_t g_timers_state = {
        .ms_ticks = 0,
};

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

static task_t g_tasks[] = {
    {"Serial Check", 1 /* ms */, service_logs},
    {"Update LCD", 250 /* ms */, update_lcd},
};

/*
 * 1ms ISR for TC0 (Red Led & Scheduler)
 */
ISR(TIMER0_COMPA_vect)
{
    g_timers_state.ms_ticks++;
    scheduler_do_schedule();
}

/*
 * Main Loop
 */
int main() {
    LOG("--------------------------------\r\n");

    timers_setup_timer(TIMER_COUNTER0, TIMER_MODE_CTC, 1000UL);
    TIMSK0 |= (1 << OCIE0A); // Unmask interrupt for output compare match A on TC0

    log_init();
	scheduler_init(&g_timers_state, g_tasks, COUNT_OF(g_tasks));

    /* enable global interrupts (do last to make timing closer) */
    sei();

	while (1) {
	    scheduler_service();
	}
}
