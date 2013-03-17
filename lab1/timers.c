#include "timers.h"
#include "leds.h"

#include <avr/interrupt.h>

static timers_state_t *g_timers_state;

void timers_init(timers_state_t * timers_state) {
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
	DDRD |= (1 << DDD5);  // set DDR5 as an output
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
 */ISR(TIMER0_COMPA_vect) {
	// This is the Interrupt Service Routine for Timer0
	// Each time the TCNT count is equal to the OCR0 register, this interrupt is "fired".

	// Increment ticks
	g_timers_state->ms_ticks++;

	// if time to toggle the RED LED, set flag to release
	if ((g_timers_state->ms_ticks % g_timers_state->red_period) == 0) {
		g_timers_state->release_red = true;
	}
}
