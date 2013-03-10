#include "LEDs.h"
#include "timer.h"
#include "usart.h"

#include <avr/io.h>
#include <stdio.h>

// RED LED is toggled in a cyclic executive defined in lab1.c
// Yellow and Green are toggled in interrupt handlers defined below.
// The interrupts are initialized in the timers.c file.
// Yellow is on Timer 3. Green is on Timer 1.

// GLOBALS
extern uint32_t G_yellow_ticks;

extern uint16_t G_red_period;
extern uint16_t G_green_period;
extern uint16_t G_yellow_period;

extern uint32_t G_red_toggles;
extern uint32_t G_green_toggles;
extern uint32_t G_yellow_toggles;

void init_LEDs() {

	int i;

	// Clear all data direction ports
>

	// Configure data direction as output
>

	// Turn LEDs on to make sure they are working
>

	for (i=0;i<200;i++)
		WAIT_10MS;

	// Start all LEDs off
>

	// clear toggle counters
	G_green_toggles = 0;
	G_red_toggles = 0;
	G_yellow_toggles = 0;
	
}

/*
void set_toggle(char color, int ms) {

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
// INTERRUPT Names are defined in iom1284p.h

// INTERRUPT HANDLER for yellow LED
> ISR(XXXX) {

	// This the Interrupt Service Routine for Toggling the yellow LED.
	// Each time the TCNT count is equal to the OCRxx register, this interrupt is enabled.
	// At creation of this file, it was initialized to interrupt every 100ms (10Hz).
	//
	// Increment ticks. If time, toggle YELLOW and increment toggle counter.
>
>
>

}

// INTERRUPT HANDLER for green LED
> ISR(XXXX) {

	// This the Interrupt Service Routine for tracking green toggles. The toggling is done in hardware.
	// Each time the TCNT count is equal to the OCRxx register, this interrupt is enabled.
	// This interrupts at the user-specified frequency for the green LED.
	
	G_green_toggles++;
}

*/
