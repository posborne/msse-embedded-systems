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
  volatile uint32_t yellow_ticks;

  /* period information */
  volatile uint16_t red_period;
  volatile uint16_t green_period;
  volatile uint16_t yellow_period;

  /* state information */
  volatile bool release_red;
} timers_state_t;

// number of empty for loops to eat up about 1 ms
#define FOR_COUNT_10MS (20000)  // TODO: find real number

uint32_t __ii;

#define WAIT_10MS {for (__ii=0;__ii<FOR_COUNT_10MS; __ii++);}

#define G_TIMER_RESOLUTION 100
#define Y_TIMER_RESOLUTION 100
#define TICKS_PER_SECOND   1000

void timers_init(timers_state_t * timers_state);
int timers_setup_timer(
        timer_counter_e timer_counter,
        timer_counter_mode_e mode,
        uint32_t target_period_microseconds);
uint32_t timers_get_uptime_ms();

#endif //__TIMER_H
