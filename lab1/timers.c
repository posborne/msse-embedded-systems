#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "leds.h"
#include "log.h"
#include "timers.h"
#include "scheduler.h"

#define COUNT_OF(x)   ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define WIDTH_8_BITS  (0x00FF)
#define WIDTH_16_BITS (0xFFFF)

#define TC0_MODE_MASK    (1 << WGM02 | 1 << WGM01 | 1 << WGM00)
#define TC0_DIVIDER_MASK (1 << CS02 | 1 << CS01 | 1 << CS00)
#define TC1_MODE_MASK    (1 << WGM12 | 1 << WGM11 | 1 << WGM10)
#define TC1_DIVIDER_MASK (1 << CS12 | 1 << CS11 | 1 << CS10)
#define TC3_MODE_MASK    (1 << WGM32 | 1 << WGM31 | 1 << WGM30)
#define TC3_DIVIDER_MASK (1 << CS32 | 1 << CS31 | 1 << CS30)


typedef void (*set_mode_t)(uint8_t mode);
typedef void (*set_divider_t)(uint8_t divider);
typedef void (*set_top_t)(uint16_t top);

static void tc0_set_mode(uint8_t mode) { TCCR0A |= (1 << WGM01); }
static void tc0_set_divider(uint8_t divider) { TCCR0B &= ~(TC0_DIVIDER_MASK); TCCR0B |= (divider & TC0_DIVIDER_MASK); }
static void tc0_set_top(uint16_t top) { OCR0A = (uint8_t)top; }

static void tc1_set_mode(uint8_t mode) { TCCR1A |= (1 << WGM12); }
static void tc1_set_divider(uint8_t divider) { TCCR1B &= ~(TC1_DIVIDER_MASK); TCCR1B |= (divider & TC1_DIVIDER_MASK); }
static void tc1_set_top(uint16_t top) { OCR1A = top; }

static void tc3_set_mode(uint8_t mode) { TCCR3B |= (1 << WGM32); }
static void tc3_set_divider(uint8_t divider) { TCCR3B &= ~(TC3_DIVIDER_MASK); TCCR3B |= (divider & TC3_DIVIDER_MASK); }
static void tc3_set_top(uint16_t top) { OCR3A = top; }


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
    clock_mode_t modes[1];

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
    .modes = {
        {TIMER_MODE_CTC, (1 << WGM02)},
    },

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
    .modes = {
        {TIMER_MODE_CTC, (1 << WGM12)},
    },

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
    .modes = {
        {TIMER_MODE_CTC, (1 << WGM32)},
    },

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
    clock_mode_t mode_info;
    int i;

    LOG(LVL_DEBUG, "Setting divider: %u, top: %u\r\n",
            divisor->denominator, top);

    /* first, set the mode */
    for (i = 0; i < COUNT_OF(timer_counter->modes); i++) {
        mode_info = timer_counter->modes[i];
        if (mode_info.mode == mode) {
            break;
        }
    }

    /* set the appropriate register via helper functions */
    timer_counter->set_mode(mode_info.mode_flags);
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

    LOG(LVL_DEBUG, "Setting up timer %s (period_ms: %lu)\r\n",
            tc->name, target_period_microseconds / 1000);

    /* find the most appropriate pre-scaler/top value */
    clock_divider_value_t * best_divisor = NULL;
    timer_counter_search_result_t best_search_result = { .delta_us = INT32_MAX };
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
        LOG(LVL_DEBUG, "Search found no workable divisor/top value pair");
        result = -1;
    } else {
        result = timers_program_timer(
            tc, best_divisor, (uint16_t)best_search_result.top_value, mode);
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
    // Timer Interrupt Using TC0
    //
    // We want the RED timer to trigger an interrupt once every millisecond
    // (that is, once every 1000 microseconds)
    //

#define USE_SETUP
#ifdef USE_SETUP
    timers_setup_timer(TIMER_COUNTER0, TIMER_MODE_CTC, 1000UL);
#else
    TCCR0A |= (1 << WGM01); // Mode = CTC
    TCCR0B |= (1 << CS02); // Clock Divider, 256
    OCR0A = 78;
#endif
    TIMSK0 |= (1 << OCIE0A); // Unmask interrupt for output compare match A on TC0

    //--------------------------- YELLOW ----------------------------------
    // Timer Interrupt Using TC3
    //
    // We want the Yellow timer to trigger an interrupt once every 100ms
    //
#ifdef USE_SETUP
    timers_setup_timer(TIMER_COUNTER3, TIMER_MODE_CTC, 100000UL);
#else
    TCCR3A |= (1 << WGM12); // CTC Mode for Timer/Counter 1
    TCCR3B |= (1 << CS12); // Clock Divider = 64

    // TOP is split over two registers as it is 16-bits
    TCNT3H = (19531 & (0xFF00)) >> 8;
    TCNT3L = (19531 & 0xFF);
#endif
    TIMSK3 |= (1 << OCIE3A); // Unmask interrupt for output compare match A on TC1

    //--------------------------- GREEN ----------------------------------
    // Timer Interrupt Using TC1
    //
    // We want the Green timer to trigger once every second.
    //
    DDRD |= (1 << DDD5); // set DDR5 as an output
    TCCR1A |= (1 << COM1A0);
#ifdef USE_SETUP
    timers_setup_timer(TIMER_COUNTER1, TIMER_MODE_CTC, 1000000UL);
#else
    TCCR1A |= (1 << WGM12 | 1 << COM1A0); // CTC Mode for Timer/Counter 1

    //TCCR1A |= (1 << COM1A0);
    TCCR1B |= (1 << CS12); // Clock Divider = 64

    // TOP is split over two registers as it is 16-bits
    TCNT1H = (19531 & (0xFF00)) >> 8;
    TCNT1L = (19531 & 0xFF);
#endif
    TIMSK1 |= (1 << OCIE1A); // Unmask interrupt for output compare match A on TC1

    /* Log key registers */
    LOG(LVL_DEBUG, "TCCR0A: 0x%02X\r\n", TCCR0A);
    LOG(LVL_DEBUG, "TCCR0B: 0x%02X\r\n", TCCR0B);
    LOG(LVL_DEBUG, "OCR0A:  0x%02X\r\n", OCR0A);

    LOG(LVL_DEBUG, "--------------\r\n");
    LOG(LVL_DEBUG, "TCCR1A: 0x%02X\r\n", TCCR1A);
    LOG(LVL_DEBUG, "TCCR1B: 0x%02X\r\n", TCCR1B);
    LOG(LVL_DEBUG, "OCR1A:  0x%04X\r\n", OCR1A);

    LOG(LVL_DEBUG, "--------------\r\n");
    LOG(LVL_DEBUG, "TCCR3A: 0x%02X\r\n", TCCR3A);
    LOG(LVL_DEBUG, "TCCR3B: 0x%02X\r\n", TCCR3B);
    LOG(LVL_DEBUG, "OCR3A:  0x%04X\r\n", OCR3A);


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

    // service the scheduler
    scheduler_do_schedule();
}
