/*
 * SENG5831: Lab Assignment 2
 */
#include <pololu/orangutan.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "timers.h"
#include "log.h"
#include "motor.h"
#include "scheduler.h"
#include "cli.h"

#define COUNT_OF(x)   ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#define CUSTOM_SYMBOL_DEGREE (3)
static const char degree_symbol[] PROGMEM = {
        0b00110,
        0b01001,
        0b00110,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000
};


static timers_state_t g_timers_state = {
        .ms_ticks = 0,
};

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
    lcd_goto_xy(0, 0);
    /* Print target degrees */
    sprintf(buf, "t:%ld", motor_get_target_pos());
    print(buf);
    lcd_goto_xy(strlen(buf), 0);
    print_character(CUSTOM_SYMBOL_DEGREE);

    /* print current degrees */
    lcd_goto_xy(0, 1);
    sprintf(buf, "c:%ld", motor_get_current_pos());
    print(buf);
    lcd_goto_xy(strlen(buf), 1);
    print_character(CUSTOM_SYMBOL_DEGREE);
}

/*
 * Task: Service CLI
 *
 * Check for new bytes and process them
 */
static void
service_cli(void)
{
    cli_service();
}

static task_t g_tasks[] = {
    {"Update LCD", 100 /* ms */, update_lcd},
    {"Service CLI", 50 /* ms */, service_cli},
    {"Service PD Controller", 25 /* ms */, motor_service_pd_controller},
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
int main()
{

    LOG("--------------------------------\r\n");

    /* Unmask interrupt for output compare match A on TC0 */
    timers_setup_timer(TIMER_COUNTER0, TIMER_MODE_CTC, 1000UL);
    TIMSK0 |= (1 << OCIE0A);

    lcd_load_custom_character(degree_symbol, CUSTOM_SYMBOL_DEGREE);

    cli_init();
    motor_init(&g_timers_state);
    log_init();
	scheduler_init(&g_timers_state, g_tasks, COUNT_OF(g_tasks));
    sei();

	/* Main Loop: Run Tasks scheduled by scheduler */
	while (1) {
	    int i;
	    for (i = 0; i < 5; i++) {
	        serial_check(); /* needs to be called frequently */
	    }
	    scheduler_service();

	}
}
