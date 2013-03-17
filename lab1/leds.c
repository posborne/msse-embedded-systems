#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "leds.h"
#include "timers.h"

static led_state_t * g_led_state;

void leds_init(led_state_t * led_state) { 
  int i;

  // Store reference to led state strcture and
  // initialize the structure
  g_led_state = led_state;
  g_led_state->green_toggles  = 0;
  g_led_state->red_toggles    = 0;
  g_led_state->yellow_toggles = 0;

  // Configure data direction as output for each LED
  DD_REG_RED    |= BIT_RED;
  DD_REG_YELLOW |= BIT_YELLOW;
  DD_REG_GREEN  |= BIT_GREEN;
  
  // Turn LEDs on to make sure they are working
  LED_ON(GREEN);
  LED_ON(RED);
  LED_ON(YELLOW);
  
  // wait 2 seconds
  for (i=0; i<200; i++) {
    WAIT_10MS
  }
  
  // Start all LEDs off
  LED_OFF(GREEN);
  LED_OFF(RED);
  LED_OFF(YELLOW);
}

/*
void leds_set_toggle(char color, int ms) {

		// check toggle ms is positive and multiple of 100
		if (ms<0) {
			printf("Cannot toggle negative ms.\n");
			return;
		}

		if (~((ms%100)==0)) {
			ms = ms - (ms%100);
			printf("Converted to toggle period: %d.\n",ms);
		}
		
		// For each color, if ms is 0, turn it off by changing data direction to input.
		// If it is >0, set data direction to output.
		if ((color=='R') || (color=='A')) {
			if (ms==0)
>				
			else
>				
			G_red_period = ms;
		}

		if ((color=='Y') || (color=='A')) {
			if (ms==0)
>				
			else
>				
			G_yellow_period = ms;
		}

		if ((color=='G') || (color=='A')) {
			if (ms==0)
>				
			else
>				

			// green has a limit on its period.
			if ( ms > 4000) ms = 4000;
			G_green_period = ms;
			
			// set the OCR1A (TOP) to get (approximately) the requested frequency.
			if ( ms > 0 ) {
>				OCR1A = 
>				printf("Green to toggle at freq %dHz (period %d ms)\n", XXXXX ,G_green_period);	
			}
 		}
}
*/


/*
 * Interrupt Service Routines
 */

// INTERRUPT HANDLER for yellow LED
ISR(TIMER1_COMPA_vect) {
	// This the Interrupt Service Routine for Toggling the yellow LED.
	// Each time the TCNT count is equal to the OCRxx register, this interrupt is enabled.
	// At creation of this file, it was initialized to interrupt every 100ms (10Hz).
	//
	// Increment ticks. If time, toggle YELLOW and increment toggle counter.
	g_led_state->yellow_toggles++;
	LED_TOGGLE(YELLOW);
}

// INTERRUPT HANDLER for green LED
ISR(TIMER3_COMPA_vect) {

	// This the Interrupt Service Routine for tracking green toggles. The toggling is done in hardware.
	// Each time the TCNT count is equal to the OCRxx register, this interrupt is enabled.
	// This interrupts at the user-specified frequency for the green LED.

	g_led_state->green_toggles++;
}
