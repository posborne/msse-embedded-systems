#include "timers.h"
#include "leds.h"

#include <avr/interrupt.h>

static timers_state_t *g_timers_state;

void timers_init(timers_state_t * timers_state)
{
  /* store timers state and initialize defaults */
  g_timers_state = timers_state;
  g_timers_state->ms_ticks = 0;
  g_timers_state->yellow_ticks = 0;
  g_timers_state->red_period = 1000;
  g_timers_state->green_period = 1000;
  g_timers_state->yellow_period = 1000;

  // -------------------------  RED --------------------------------------//
  // Software Clock Using Timer/Counter 0.
  // THE ISR for this is below.
  TCCR0A |= (1 << WGM01);  // Mode = CTC
  TCCR0B |= (1 << CS02 | 1 << CS00);  // Clock Divider, 1024
  OCR0A = 20; // set top to be at 20
  TIMSK0 |= (1 << OCIE0A); // Unmkask interrupt for output compare match A

  //--------------------------- YELLOW ----------------------------------//
  // Set-up of interrupt for toggling yellow LEDs. 
  // This task is "self-scheduled" in that it runs inside the ISR that is 
  // generated from a COMPARE MATCH of 
  //      Timer/Counter 1 to OCR3A.
  // Obviously, we could use a single timer to schedule everything, but we are experimenting here!
  // THE ISR for this is in the LEDs.c file
  
/*
	// SET appropriate bits in TCCR ...

	// Using CTC mode with OCR3A for TOP. This is mode XX, thus WGM1/3210 = .
>
	
	// Using pre-scaler XX. This is CS1_210 = 
>


	// Interrupt Frequency: 10 = f_IO / (prescaler*OCR3A)
	// Set OCR3A appropriately for TOP to generate desired frequency using Y_TIMER_RESOLUTION (100 ms).
	// NOTE: This is not the toggle frequency, rather a tick frequency used to time toggles.
>	OCR3A = 
>	printf("Initializing yellow clock to freq %dHz (period %d ms)\n",(int)(XXX),Y_TIMER_RESOLUTION);	

	//Enable output compare match interrupt on timer 3A
>

	G_yellow_ticks = 0;
*/
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

}

/*
//INTERRUPT HANDLERS
> ISR(XXXX) {

	// This is the Interrupt Service Routine for Timer0
	// Each time the TCNT count is equal to the OCR0 register, this interrupt is "fired".

	// Increment ticks
	G_ms_ticks++;

	// if time to toggle the RED LED, set flag to release
	if ( ( G_ms_ticks % G_red_period ) == 0 )
		G_release_red = 1;
}
*/
