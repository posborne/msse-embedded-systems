#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "timers.h"
#include "leds.h"
#include "log.h"

#define COUNT_OF(x)   ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define WIDTH_8_BITS  (0x00FF)
#define WIDTH_16_BITS (0xFFFF)

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
    uint8_t top_register_ioaddr_8;
    uint16_t top_register_ioaddr_16;
    uint16_t width;
} top_info_t;

typedef struct {
    timer_counter_e id;
    char * name;

    uint8_t * divider_register_ioaddr;
    uint8_t divider_mask;
    clock_divider_value_t divisors[5];

    uint8_t * mode_register_ioaddr;
    uint8_t mode_mask;
    clock_mode_t modes[1];

    top_info_t top_info;
} timer_counter_t;

/* 8-bit Timer/Counter0 */
static timer_counter_t tc0 = {
    .id = TIMER_COUNTER0,
    .name = "TIMER_COUNTER0",

    .divider_register_ioaddr = (uint8_t *)0x25, // TCCR0B
    .divider_mask = (1 << CS02 | 1 << CS01 | 1 << CS00),
    .divisors = {
         {1, (1 << CS00)},
         {8, (1 << CS01)},
         {64, (1 << CS01 | 1 << CS00)},
         {256, (1 << CS02)},
         {1024, (1 << CS02 | 1 << CS00)},
    },

    .mode_register_ioaddr = (uint8_t *)0x80, /* TCCR1A */
    .mode_mask = (1 << WGM02 || 1 << WGM01 || 1 << WGM00),
    .modes = {
        {TIMER_MODE_CTC, (1 << WGM02)},
    },

    .top_info = {
        .top_register_ioaddr_8 = 0x27, // OCR0A
        .width = WIDTH_8_BITS
    }
};

/* 16-bit Timer/Counter1 */
static timer_counter_t tc1 = {
    .id = TIMER_COUNTER1,
    .name = "TIMER_COUNTER1",

    .divider_register_ioaddr = (uint8_t *)0x81, // TCCR1B
    .divider_mask = (1 << CS12 | 1 << CS11 | 1 << CS10),
    .divisors = {
        {1, (1 << CS10)},
        {8, (1 << CS11)},
        {64, (1 << CS11 | 1 << CS10)},
        {256, (1 << CS12)},
        {1024, (1 << CS12 | 1 << CS10)},
    },

    .mode_register_ioaddr = (uint8_t *)0x80, // TCCR1A,
    .mode_mask = (1 << WGM12 || 1 << WGM11 || 1 << WGM10),
    .modes = {
        {TIMER_MODE_CTC, (1 << WGM12)},
    },

    .top_info = {
        .top_register_ioaddr_16 = 0x88, // OCR1A
        .width = WIDTH_16_BITS
    }
};

/* 8-bit Timer/Counter2 */
static timer_counter_t tc2 = {
    .id = TIMER_COUNTER2,
    .name = "TIMER_COUNTER2",

    .divider_register_ioaddr = (uint8_t *)0xB1, // TCCR2B,
    .divider_mask = (1 << CS22 | 1 << CS21 | 1 << CS20),
    .divisors = {
        {1, (1 << CS20)},
        {8, (1 << CS21)},
        {64, (1 << CS21 | 1 << CS20)},
        {256, (1 << CS22)},
        {1024, (1 << CS22 | 1 << CS20)},
    },

    .mode_register_ioaddr = (uint8_t *)0xB0, // TCCR2A,
    .mode_mask = (1 << WGM22 || 1 << WGM21 || 1 << WGM20),
    .modes = {
        {TIMER_MODE_CTC, (1 << WGM22)},
    },

    .top_info = {
        .top_register_ioaddr_8 = 0xB3, // OCR2A
        .width = WIDTH_8_BITS
    }
};

/* 16-bit Timer/Counter3 */
static timer_counter_t tc3 = {
    .id = TIMER_COUNTER3,
    .name = "TIMER_COUNTER3",

    .divider_register_ioaddr = (uint8_t *)0x91, // TCCR3B
    .divider_mask = (1 << CS32 | 1 << CS31 | 1 << CS30),
    .divisors = {
        {1, (1 << CS30)},
        {8, (1 << CS31)},
        {64, (1 << CS31 | 1 << CS30)},
        {256, (1 << CS32)},
        {1024, (1 << CS32 | 1 << CS30)},
    },

    .mode_register_ioaddr = (uint8_t *)0x90, // TCCR3A
    .mode_mask = (1 << WGM32 || 1 << WGM31 || 1 << WGM30),
    .modes = {
        {TIMER_MODE_CTC, (1 << WGM32)},
    },

    .top_info = {
        .top_register_ioaddr_16 = 0x98, // OCR3A
        .width = WIDTH_16_BITS
    }
};

static timers_state_t *g_timers_state;
static timer_counter_t * timer_counters[] = {&tc0, &tc1, &tc2, &tc3};

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
//    LOG(LVL_DEBUG, " > find_top_value(period_us=%lu, divisor=%u, width=0x%04X)",
//            target_period_us, clock_divisor, width);
    top = (uint64_t)((20 * target_period_us) / clock_divisor);
    if (top > width) {
//        LOG(LVL_DEBUG, "   match not found!");
        result.match_found = false;
        result.top_value = 0;
        result.delta_us = 0;
    } else {
        uint32_t achieved_period_us;
        achieved_period_us = (clock_divisor * top) / 20;
        result.match_found = true;
        result.top_value = (uint32_t)top;
        result.delta_us = (uint32_t)abs(achieved_period_us - target_period_us);
//        LOG(LVL_DEBUG, "   match found, top: %ld, achieved_period_us: %ld, "
//                       "delta_us: %lu",
//                       result.top_value, achieved_period_us, result.delta_us);
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

    LOG(LVL_DEBUG, "Setting divider: %u, top: %u",
            divisor->denominator, top);

    /* first, set the mode */
    for (i = 0; i < COUNT_OF(timer_counter->modes); i++) {
        mode_info = timer_counter->modes[i];
        if (mode_info.mode == mode) {
            break;
        }
    }


    _SFR_MEM8((uint8_t *)timer_counter->mode_register_ioaddr) &= ~(timer_counter->mode_mask);
    _SFR_MEM8((uint8_t *)timer_counter->mode_register_ioaddr) |= (mode_info.mode_flags);
//    TCCR1B &= ~(timer_counter->mode_mask);
//    TCCR1B |= (mode_info.mode_flags);

    /* now set the pre-scaler */
    _SFR_MEM8(timer_counter->divider_register_ioaddr) &= (timer_counter->divider_mask);
    _SFR_MEM8(timer_counter->divider_register_ioaddr) |= (divisor->clock_select_flags);

    /* now, set the top value */
    tinfo = timer_counter->top_info;
    if (tinfo.width == WIDTH_8_BITS) {
        _SFR_IO8(tinfo.top_register_ioaddr_8) = (uint8_t)(divisor->denominator & 0xFF);
    } else {
        _SFR_MEM16(tinfo.top_register_ioaddr_16) = (uint16_t)(divisor->denominator);
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
        tc = timer_counters[i];
        if (tc->id == timer_counter) {
            break;
        }
    }

    LOG(LVL_DEBUG, "Setting up timer %s (period_ms: %lu)",
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
    // Software Clock Using Timer/Counter 0.
    // THE ISR for this is below.
    timers_setup_timer(TIMER_COUNTER0, TIMER_MODE_CTC, 1000);
    TIMSK0 |= (1 << OCIE0A); // Unmask interrupt for output compare match A on TC0

    //--------------------------- YELLOW ----------------------------------//
    // Set-up of interrupt for toggling yellow LEDs.
    //
    // For yellow, we will be using Timer/Counter 3
    timers_setup_timer(TIMER_COUNTER3, TIMER_MODE_CTC, 100000);
    TIMSK3 |= (1 << OCIE3A); // Unmask interrupt for output compare match A on TC1

    //--------------------------- GREEN ----------------------------------//
    DDRD |= (1 << DDD5); // set DDR5 as an output
    timers_setup_timer(TIMER_COUNTER1, TIMER_MODE_CTC, 1000000);
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
