/*******************************************
*
* Header file for Timer stuff.
*
*******************************************/
#ifndef __TIMER_H
#define __TIMER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TIMER_COUNTER0,
    TIMER_COUNTER1,
    TIMER_COUNTER2,
    TIMER_COUNTER3
} timer_counter_e;

typedef enum {
    TIMER_MODE_CTC
} timer_counter_mode_e;

typedef struct {
  /* ticks */
  volatile uint32_t ms_ticks;
} timers_state_t;

int timers_setup_timer(
        timer_counter_e timer_counter,
        timer_counter_mode_e mode,
        uint32_t target_period_microseconds);
uint32_t timers_get_uptime_ms();

#endif //__TIMER_H
