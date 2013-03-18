#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "timers.h"
#include "leds.h"

#define COUNT_OF(x)   ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define WIDTH_8_BITS  (0x00FF)
#define WIDTH_16_BITS (0xFFFF)

typedef struct {
    bool match_found;
    uint32_t top_value;
    int32_t delta_us;
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
    union {
        uint16_t * reg16;
        uint8_t * reg8;
    } top_register;
    uint16_t width;
} top_info_t;

typedef struct {
    timer_counter_e id;

    uint8_t * divider_register;
    uint8_t divider_mask;
    clock_divider_value_t divisors[];

    uint8_t * mode_register;
    uint8_t mode_mask;
    clock_mode_t modes[];

    top_info_t top_info;
} timer_counter_t;

/* 8-bit Timer/Counter0 */
static timer_counter_t tc0 = {
    .id = TIMER_COUNTER0,

    .divider_register = TCCR0B,
    .divider_mask = (1 << CS02 | 1 << CS01 | 1 << CS00),
    .divisors = {
         {1, (1 << CS00)},
         {8, (1 << CS01)},
         {64, (1 << CS01 | 1 << CS00)},
         {256, (1 << CS02)},
         {1024, (1 << CS02 | 1 << CS00)},
         NULL
    },

    .mode_register = TCCR1A,
    .mode_mask = (1 << WGM02 || 1 << WGM01 || 1 << WGM00),
    .modes = {
        {TIMER_MODE_CTC, (1 << WGM02)},
        NULL
    },

    .top_info = {
        .top_register = {.reg8 = OCR0A},
        .width = WIDTH_8_BITS
    }
};

/* 16-bit Timer/Counter1 */
static timer_counter_t tc1 = {
    .id = TIMER_COUNTER1,

    .divider_register = TCCR1B,
    .divider_mask = (1 << CS12 | 1 << CS11 | 1 << CS10),
    .divisors = {
        {1, (1 << CS10)},
        {8, (1 << CS11)},
        {64, (1 << CS11 | 1 << CS10)},
        {256, (1 << CS12)},
        {1024, (1 << CS12 | 1 << CS10)},
        NULL
    },

    .mode_register = TCCR1A,
    .mode_mask = (1 << WGM12 || 1 << WGM11 || 1 << WGM10),
    .modes = {
        {TIMER_MODE_CTC, (1 << WGM12)},
        NULL
    },

    .top_info = {
        .top_register = {.reg16 = OCR1A},
        .width = WIDTH_16_BITS
    }
};

/* 8-bit Timer/Counter2 */
static timer_counter_t tc2 = {
    .id = TIMER_COUNTER2,

    .divider_register = TCCR2B,
    .divider_mask = (1 << CS22 | 1 << CS21 | 1 << CS20),
    .divisors = {
        {1, (1 << CS20)},
        {8, (1 << CS21)},
        {64, (1 << CS21 | 1 << CS20)},
        {256, (1 << CS22)},
        {1024, (1 << CS22 | 1 << CS20)},
        NULL
    },

    .mode_register = TCCR2A,
    .mode_mask = (1 << WGM22 || 1 << WGM21 || 1 << WGM20),
    .modes = {
        {TIMER_MODE_CTC, (1 << WGM22)},
        NULL
    },

    .top_info = {
        .top_register = {.reg8 = OCR2A},
        .width = WIDTH_8_BITS
    }
};

/* 16-bit Timer/Counter3 */
static timer_counter_t tc3 = {
    .id = TIMER_COUNTER3,

    .divider_register = TCCR3B,
    .divider_mask = (1 << CS32 | 1 << CS31 | 1 << CS30),
    .divisors = {
        {1, (1 << CS30)},
        {8, (1 << CS31)},
        {64, (1 << CS31 | 1 << CS30)},
        {256, (1 << CS32)},
        {1024, (1 << CS32 | 1 << CS30)},
        NULL
    },

    .mode_register = TCCR3A,
    .mode_mask = (1 << WGM32 || 1 << WGM31 || 1 << WGM30),
    .modes = {
        {TIMER_MODE_CTC, (1 << WGM32)},
        NULL
    },

    .top_info = {
        .top_register = {.reg16 = OCR3A},
        .width = WIDTH_16_BITS
    }
};

static timers_state_t *g_timers_state;
static timer_counter_t timer_counters[] = {tc0, tc1, tc2, tc3};

/*
 * Here's some basic math for guiding this code
 * --------------------------------------------
 * frequency_hz = cpu_clock_hz / (clock_divider * top)
 * period_s = 1 / frequency_hz = (clock_divider * top) / cpu_clock_hz
 * period_us = (1000000 * clock_divider * top) / cpu_clock_hz
 * top = (1000000 * clock_divider) / (cpu_clock_hz * period_us)
 */
static timer_counter_search_result_t find_top_value(
        uint32_t target_period_us,
        uint16_t clock_divisor,
        uint16_t width)
{
    timer_counter_search_result_t result;
    uint32_t top;
    uint32_t achieved_period_us;
    top = (1000000 * clock_divisor) / (F_CPU * target_period_us);
    achieved_period_us = (1000000 * clock_divisor * top) / (F_CPU);
    if (top > width) {
        result.match_found = false;
        result.top_value = 0;
        result.delta_us = 0;
    } else {
        result.match_found = true;
        result.top_value = top;
        result.delta_us = (achieved_period_us - target_period_us);
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
    clock_mode_t mode_info;
    int i;
    top_info_t tinfo;

    /* first, set the mode */
    i = 0;
    while ((mode_info = timer_counter->modes[i++]) != NULL) {
        if (mode_info.mode == mode)
            break;
    }
    (*timer_counter->mode_register) &= ~(timer_counter->mode_mask);
    (*timer_counter->mode_register) |= (mode_info.mode_flags);

    /* now set the pre-scaler */
    (*timer_counter->divider_register) &= (timer_counter->divider_mask);
    (*timer_counter->divider_register) |= (divisor->clock_select_flags);

    /* now, set the top value */
    tinfo = timer_counter->top_info;
    if (tinfo.width == WIDTH_8_BITS) {
        (*tinfo.top_register) = (uint8_t)(divisor->denominator & 0xFF);
    } else {
        (*tinfo.top_register) = (uint16_t)(divisor->denominator);
    }
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
        tc = &timer_counters[i];
        if (tc->id == timer_counter) {
            break;
        }
    }

    /* find the most appropriate pre-scaler/top value */
    clock_divider_value_t * best_divisor = NULL;
    timer_counter_search_result_t best_search_result = NULL;
    i = 0;
    while ((divisor = tc->divisors[i++]) != NULL) {
        search_result = find_top_value(
                target_period_microseconds,
                divisor->denominator,
                tc->top_info.width);
        if (search_result.match_found) {
            if ((best_search_result == NULL) ||
                    (abs(search_result.delta_us) < abs(best_search_result.delta_us))) {
                best_search_result = search_result;
                best_divisor = divisor;
            }
        }
    }

    if (best_divisor == NULL) {
        result = -1;
    } else {
        result = timers_program_timer(tc, best_divisor, best_search_result.top_value, mode);
    }

    return result;
}

/*
 * Initialize the timers for the system.
 */
void timers_init(timers_state_t * timers_state)
{
    /* store timers state and initialize defaults */
    g_timers_state = timers_state;
    g_timers_state->ms_ticks = 0;
    g_timers_state->yellow_ticks = 0;
    g_timers_state->red_period = 1000;
    g_timers_state->green_period = 1000;
    g_timers_state->yellow_period = 1000;
    g_timers_state->release_red = false;

    // -------------------------  RED --------------------------------------//
    // Software Clock Using Timer/Counter 0.
    // THE ISR for this is below.
    //
    // This timer should have a 1ms resolution (1000hz).  To achieve this, we will
    // be using an 8-bit timer.  Let's start with a clock divider of 1024 (we need
    // to use the same divider for our 1ms and 10ms timers).
    //
    // frequency(hz) = (20000000) / (clock_divider * top)
    // 1000 = (20000000) / (256 * top)
    // 256000 = (20000000) / top
    // top = 20000000 / 256000
    // top =~ 78 (78.125)
    //
    // This results in a clock where each tick has a period
    // p = 1000 / (20000000 / (256 * 78))
    //   = ~0.9984 ms
    //
    // close enough.  Over an hour, we will be behind by ~5.76 seconds.  This is
    // not a wrist-watch.
    TCCR0A |= (1 << WGM01); // Mode = CTC
    TCCR0B |= (1 << CS02); // Clock Divider, 256
    OCR0A = 78; // set top to be at 20
    TIMSK0 |= (1 << OCIE0A); // Unmask interrupt for output compare match A on TC0

    //--------------------------- YELLOW ----------------------------------//
    // Set-up of interrupt for toggling yellow LEDs.
    //
    // This task is "self-scheduled" in that it runs inside the ISR that is
    // generated from a COMPARE MATCH of
    //
    // For yellow, we will be using Timer/Counter 3
    //
    // top = 20000000 / (freq_hz * clock_divider)
    //
    // with, divider = 64, freq_hz = 10
    //
    // top = 20000000 / (10 * 64)
    //     = 32
    TCCR3A |= (1 << WGM12); // CTC Mode for Timer/Counter 1
    TCCR3B |= (1 << CS12); // Clock Divider = 64

    // TOP is split over two registers as it is 16-bits
    TCNT3H = (19531 & (0xFF00)) >> 8;
    TCNT3L = (19531 & 0xFF);
    TIMSK3 |= (1 << OCIE3A); // Unmask interrupt for output compare match A on TC1

    //--------------------------- GREEN ----------------------------------//
    // Set-up of interrupt for toggling green LED.
    // This "task" is implemented in hardware, because the OC1A pin will be toggled when
    // a COMPARE MATCH is made of
    //
    //      Timer/Counter 1 to OCR1A.
    //
    // frequency(hz) = (20000000) / (clock_divider * top)
    // 1 = (20000000) / (1024 * top)
    // 1024 = 20000000 / top
    // top = 20000000 / 1024
    // top = 19531
    DDRD |= (1 << DDD5); // set DDR5 as an output
    TCCR1A |= (1 << WGM12 | 1 << COM1A0); // CTC Mode for Timer/Counter 1

    //TCCR1A |= (1 << COM1A0);
    TCCR1B |= (1 << CS12); // Clock Divider = 64

    // TOP is split over two registers as it is 16-bits
    TCNT1H = (19531 & (0xFF00)) >> 8;
    TCNT1L = (19531 & 0xFF);
    TIMSK1 |= (1 << OCIE1A); // Unmask interrupt for output compare match A on TC1

    /* enable global interrupts */
    sei();
}

/*
 * Interrupt Service Routines
 */
ISR(TIMER0_COMPA_vect)
{
    // This is the Interrupt Service Routine for Timer0
    // Each time the TCNT count is equal to the OCR0 register, this interrupt is "fired".

    // Increment ticks
    g_timers_state->ms_ticks++;

    // if time to toggle the RED LED, set flag to release
    if ((g_timers_state->ms_ticks % g_timers_state->red_period) == 0) {
        g_timers_state->release_red = true;
    }
}
