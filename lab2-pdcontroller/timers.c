#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "log.h"
#include "timers.h"
#include "scheduler.h"

#define COUNT_OF(x)   ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define WIDTH_8_BITS  (0x00FF)
#define WIDTH_16_BITS (0xFFFF)
#define MS_TO_uS(x)   (x * 1000UL)

#define TC0_DIVIDER_MASK (1 << CS02 | 1 << CS01 | 1 << CS00)
#define TC1_DIVIDER_MASK (1 << CS12 | 1 << CS11 | 1 << CS10)
#define TC3_DIVIDER_MASK (1 << CS32 | 1 << CS31 | 1 << CS30)

typedef void (*set_mode_t)(timer_counter_mode_e mode);
typedef void (*set_divider_t)(uint8_t divider);
typedef void (*set_top_t)(uint16_t top);

/*
 * TC0 Setters
 */
static void tc0_set_mode(timer_counter_mode_e mode) {
    if (mode == TIMER_MODE_CTC) {
        TCCR0A &= ~(1 << WGM01 | 1 << WGM02);
        TCCR0B &= ~(1 << WGM02);
        TCCR0A |= (1 << WGM01);
    }
}
static void tc0_set_divider(uint8_t divider) {
    TCCR0B &= ~(TC0_DIVIDER_MASK);
    TCCR0B |= (divider & TC0_DIVIDER_MASK);
}
static void tc0_set_top(uint16_t top) {
    OCR0A = (uint8_t)top;
}

/*
 * TC1 Setters
 */
static void tc1_set_mode(timer_counter_mode_e mode) {
    if (mode == TIMER_MODE_CTC) {
        TCCR1A &= ~(1 << WGM10 | 1 << WGM11);
        TCCR1B |= (1 << WGM12);
    }
}
static void tc1_set_divider(uint8_t divider) {
    TCCR1B &= ~(TC1_DIVIDER_MASK);
    TCCR1B |= (divider & TC1_DIVIDER_MASK);
}
static void tc1_set_top(uint16_t top) {
    OCR1A = top;
}

/*
 * TC3 Setters
 */
static void tc3_set_mode(timer_counter_mode_e mode) {
    if (mode == TIMER_MODE_CTC) {
        TCCR3A &= ~(1 << WGM30 | 1 << WGM31);
        TCCR3B |= (1 << WGM32);
    }
}
static void tc3_set_divider(uint8_t divider) {
    TCCR3B &= ~(TC3_DIVIDER_MASK);
    TCCR3B |= (divider & TC3_DIVIDER_MASK);
}
static void tc3_set_top(uint16_t top) {
    OCR3A = top;
}


typedef struct {
    bool match_found;
    uint32_t top_value;
    uint32_t delta_us;
} timer_counter_search_result_t;

typedef struct {
    uint16_t denominator;
    uint8_t clock_select_flags;
} clock_divider_value_t;

typedef struct {
    timer_counter_mode_e mode;
    uint8_t mode_flags;
} clock_mode_t;

typedef struct {
    set_top_t set_top;
    uint16_t width;
} top_info_t;

typedef struct {
    timer_counter_e id;
    char * name;
    set_divider_t set_divider;
    clock_divider_value_t divisors[5];
    set_mode_t set_mode;
    top_info_t top_info;
} timer_counter_t;

/* 8-bit Timer/Counter0 */
static timer_counter_t tc0 = {
    .id = TIMER_COUNTER0,
    .name = "TIMER_COUNTER0",
    .set_divider = tc0_set_divider,
    .divisors = {
         {1, (1 << CS00)},
         {8, (1 << CS01)},
         {64, (1 << CS01 | 1 << CS00)},
         {256, (1 << CS02)},
         {1024, (1 << CS02 | 1 << CS00)},
    },
    .set_mode = tc0_set_mode,
    .top_info = {
        .set_top = tc0_set_top,
        .width = WIDTH_8_BITS
    }
};

/* 16-bit Timer/Counter1 */
static timer_counter_t tc1 = {
    .id = TIMER_COUNTER1,
    .name = "TIMER_COUNTER1",
    .set_divider = tc1_set_divider,
    .divisors = {
        {1, (1 << CS10)},
        {8, (1 << CS11)},
        {64, (1 << CS11 | 1 << CS10)},
        {256, (1 << CS12)},
        {1024, (1 << CS12 | 1 << CS10)},
    },
    .set_mode = tc1_set_mode,
    .top_info = {
        .set_top = tc1_set_top,
        .width = WIDTH_16_BITS
    }
};

/* 16-bit Timer/Counter3 */
static timer_counter_t tc3 = {
    .id = TIMER_COUNTER3,
    .name = "TIMER_COUNTER3",
    .set_divider = tc3_set_divider,
    .divisors = {
        {1, (1 << CS30)},
        {8, (1 << CS31)},
        {64, (1 << CS31 | 1 << CS30)},
        {256, (1 << CS32)},
        {1024, (1 << CS32 | 1 << CS30)},
    },
    .set_mode = tc3_set_mode,
    .top_info = {
        .set_top = tc3_set_top,
        .width = WIDTH_16_BITS
    }
};

static timers_state_t *g_timers_state;
static timer_counter_t * timer_counters[] = {&tc0, &tc1, &tc3};

/*
 * Give a target period, divisor and width try to find optimal value
 */
static timer_counter_search_result_t find_top_value(
        uint32_t target_period_us,
        uint16_t clock_divisor,
        uint16_t width)
{
    timer_counter_search_result_t result;
    uint32_t top;
    top = (uint64_t)((20 * target_period_us) / clock_divisor);
    if (top > width) {
        result.match_found = false;
        result.top_value = 0;
        result.delta_us = 0;
    } else {
        uint32_t achieved_period_us;
        achieved_period_us = (clock_divisor * top) / 20;
        result.match_found = true;
        result.top_value = (uint32_t)top;
        result.delta_us = (uint32_t)abs(achieved_period_us - target_period_us);
    }
    return result;
}

/*
 * Private Method
 *
 * Program a timer with a set of values.
 */
static int timers_program_timer(
        timer_counter_t * timer_counter,
        clock_divider_value_t * divisor,
        uint16_t top,
        timer_counter_mode_e mode)
{
    LOG("Setting divider: %u, top: %u\r\n", divisor->denominator, top);
    timer_counter->set_mode(mode);
    timer_counter->set_divider(divisor->clock_select_flags);
    timer_counter->top_info.set_top(top);
    return 0;
}

/*
 * Get the number of milliseconds the system has been alive
 *
 * In actuality, there is a small amount time between when
 * we get power and when we start counting.  This doesn't really
 * matter in practice.
 */
uint32_t timers_get_uptime_ms()
{
    return g_timers_state->ms_ticks;
}

/*
 * Setup (or change the setup for) a timer.  An attempt will
 * be made to setup the timer to match the target period as
 * closely as possible.
 *
 * If a timer cannot be setup that is close (e.g. in the
 * feasible range) a negative value will be returned.
 */
int timers_setup_timer(
        timer_counter_e timer_counter,
        timer_counter_mode_e mode,
        uint32_t target_period_microseconds)
{
    int i;
    int result;
    timer_counter_t * tc;
    clock_divider_value_t * divisor;
    timer_counter_search_result_t search_result;

    /* get a reference to our timer/counter.  Since we should have
     * full coverage of timer/counters in timer_counter_e, we do not
     * handle an error case.
     */
    for (i = 0; i < COUNT_OF(timer_counters); i++) {
        tc = timer_counters[i];
        if (tc->id == timer_counter) {
            break;
        }
    }

    LOG("Setting up timer %s (period_ms: %lu)\r\n",
            tc->name, target_period_microseconds / 1000);

    /* find the most appropriate pre-scaler/top value */
    clock_divider_value_t * best_divisor = NULL;
    timer_counter_search_result_t best_search_result = { .delta_us = UINT32_MAX };
    for (i = 0; i < COUNT_OF(tc->divisors); i++) {
        divisor = &tc->divisors[i];
        search_result = find_top_value(
                target_period_microseconds,
                divisor->denominator,
                tc->top_info.width);
        if (search_result.match_found) {
            if (search_result.delta_us < best_search_result.delta_us) {
                best_search_result = search_result;
                best_divisor = divisor;
            }
        }
    }

    if (best_divisor == NULL) {
        LOG("Search found no workable divisor/top value pair\r\n");
        result = -1;
    } else {
        result = timers_program_timer(
            tc, best_divisor, (uint16_t)best_search_result.top_value, mode);
    }

    return result;
}
