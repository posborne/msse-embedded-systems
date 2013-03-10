#include "LEDs.h"
#include "timer.h"
#include "usart.h"
#include "menu.h"

#include <avr/io.h>         //gives us names for registers
#include <avr/interrupt.h>

//Gives us uintX_t (e.g. uint32_t - unsigned 32 bit int)
//On the ATMega128 int is actually 16 bits, so it is better to use
//  the int32_t or int16_t so you know exactly what is going on
#include <inttypes.h> //gives us uintX_t

// useful stuff from libc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// GLOBALS
volatile uint32_t G_yellow_ticks = 0;
volatile uint32_t G_ms_ticks = 0;

volatile uint16_t G_red_period = 1000;
volatile uint16_t G_green_period = 1000;
volatile uint16_t G_yellow_period = 1000;

volatile uint16_t G_release_red = 0;

volatile uint32_t G_red_toggles = 0;
volatile uint32_t G_green_toggles = 0;
volatile uint32_t G_yellow_toggles = 0;

volatile uint8_t G_flag = 0; // generic flag for debugging

int main(void) {
	// -------------------------------------------------------------
	// This program teaches you about the various ways to "schedule" tasks.
	// You can think of the three blinking LEDs as separate tasks with 
	// strict timing constraints.
	//
	// As you build your program, think about the guarantees that you can 
	// make regarding the meeting of deadlines. Think about how the CPU
	// is "communicating" with the LEDs. Obviously, everything is output,
	// but what if it was input? Also think about how data can be passed
	// among tasks and how conflicts might arise.
	//
	// You will construct this program in pieces.
	// First, establish WCET analysis on a for loop to use for timing.
	// Use the for loop to blink the red LED.
	// Next, set up a software timer, and use that to "schedule" the blink
	// inside a cyclic executive.
	//
	// Blink the yellow LED using a separate timer with a different resolution
	// from the red LED. Blink the LED inside the ISR.
	//
	// Finally, blink the green LED by toggling the output on a pin using
	// a Compare Match. 
	//	
	// --------------------------------------------------------------

	int i;
	
	//communication: initialize serial port 1, and open up stdout/stdin
	init_USART1();
	fdevopen(USART1_stdio_send, USART1_stdio_get);

	// initialize everything else
	init_LEDs();
	init_timers();
	init_menu();
	
	//enable interrupts
	sei();
		    
	while (1) {

		/* BEGIN with a simple toggle using for-loops. No interrupt timers */

		// toggle the LED. Increment a counter.
		LED_TOGGLE(RED);
		G_red_toggles++;
		printf("%d ",G_red_toggles);

		// create a for-loop to kill approximately 1 second
		for (i=0;i<100;i++) {
			WAIT_10MS;
		}

		// ONCE THAT WORKS, Comment out the above and use a software timer
		//	to "schedule" the RED LED toggle.
/*
		if (G_release_red) {
			LED_TOGGLE(RED);
			G_red_toggles++;
			G_release_red = 0; 
		}
*/
/*
		// Whenever you are ready, add in the menu task.
		// Think of this as an external interrupt "releasing" the task.
		if(USART1_input_ready()) {
			menu();
		}
*/
					
	} //end while loop
} //end main

