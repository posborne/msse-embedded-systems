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
    char tmp[8];
    clear();
    lcd_goto_xy(0, 0);

    /* Print target/actual degrees */
    sprintf(buf, "(%-5ld , %-5ld )", motor_get_target_pos(), motor_get_current_pos());
    print(buf);
    sprintf(tmp, "%ld", motor_get_target_pos());
    lcd_goto_xy(1 + strlen(tmp), 0);
    print_character(CUSTOM_SYMBOL_DEGREE);
    sprintf(tmp, "%ld", motor_get_current_pos());
    lcd_goto_xy(9 + strlen(tmp), 0);
    print_character(CUSTOM_SYMBOL_DEGREE);

    /* Print last torque value */
    lcd_goto_xy(0, 1);
    sprintf(buf, "torque: %d", motor_get_last_torque());
    print(buf);
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
    {"Log Motor State", 50 /* ms */, motor_log_state},
    {"Service PD (5Hz)", 200 /* ms */, motor_service_pd_controller_5hz},
    {"Service PD (50Hz)", 20 /* ms */, motor_service_pd_controller_50hz},
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
