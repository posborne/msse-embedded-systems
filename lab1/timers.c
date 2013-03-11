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
	TIMSK0 |= (1 << OCIE0A); // Unmkask interrupt for output compare match A on TC0

	//--------------------------- YELLOW ----------------------------------//
	// Set-up of interrupt for toggling yellow LEDs.
	//
	// This task is "self-scheduled" in that it runs inside the ISR that is
	// generated from a COMPARE MATCH of
	//      Timer/Counter 1 to OCR3A.
	// Obviously, we could use a single timer to schedule everything, but we are experimenting here!
	// THE ISR for this is in the LEDs.c file
	//
	// For this timer, we will be using Timer/Counter1 (which has no PWM support).  We will
	// be using the timer in CTC (Clear Timer on Compare) mode and setting an appropriate
	// prescaler divider and TOP in order to get as close to 10ms as possible for the
	// interrupt period.
	//
	// frequency(hz) = (20000000) / (clock_divider * top)
	// 10 = (20000000) / (64 * top)
	// 640 = 20000000 / top
	// top = 20000000 / 640
	// top = 31250
	//
	// This results in a clock where each tick has a period
	// p = 1000 / (20000000 / (64 * 31250))
	//   = 100 ms
	//
	// Over an hour, the clock will be behind by ~5.76 seconds.
	TCCR1A |= (1 << WGM12); // CTC Mode for Timer/Counter 1
	TCCR1B |= (1 << CS11 | 1 << CS10); // Clock Divider = 64

	// TOP is split over two registers as it is 16-bits
	TCNT1H = (32150 & (0xFF00)) >> 8;
	TCNT1L = (31250 & 0xFF);
	TIMSK1 |= (1 << OCIE1A); // Unmask interrupt for output compare match A on TC1

	/*
	 //--------------------------- GREEN ----------------------------------//
	 // Set-up of interrupt for toggling green LED.
	 // This "task" is implemented in hardware, because the OC1A pin will be toggled when
	 // a COMPARE MATCH is made of
	 //      Timer/Counter 1 to OCR1A.
	 // We will keep track of the number of matches (thus toggles) inside the ISR (in the LEDs.c file)
	 // Limits are being placed on the frequency because the frequency of the clock
	 // used to toggle the LED is limited.

	 // Using CTC mode with OCR1A for TOP. This is mode XX, thus WGM3/3210 = .
	 >

	 // Toggle OC1A on a compare match. Thus COM1A_10 = 01
	 >

	 // Using pre-scaler 1024. This is CS1/2/1/0 = XXX
	 >

	 // Interrupt Frequency: ? = f_IO / (1024*OCR1A)
	 // Set OCR1A appropriately for TOP to generate desired frequency.
	 // NOTE: This IS the toggle frequency.
	 printf("green period %d\n",G_green_period);
	 >	OCR1A = (uint16_t) (XXXX);
	 printf("Set OCR1A to %d\n",OCR1A);
	 >	printf("Initializing green clock to freq %dHz (period %d ms)\n",(int)(XXXX),G_green_period);

	 // A match to this will toggle the green LED.
	 // Regardless of its value (provided it is less than OCR3A), it will match at the frequency of timer 3.
	 OCR1B = 1;

	 //Enable output compare match interrupt on timer 1B
	 >
	 */

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
