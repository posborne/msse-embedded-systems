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
 * Interrupt Service Routines
 */

// INTERRUPT HANDLER for yellow LED
ISR(TIMER1_COMPA_vect) {
    // This the Interrupt Service Routine for tracking green toggles. The toggling is done in hardware.
    // Each time the TCNT count is equal to the OCRxx register, this interrupt is enabled.
    // This interrupts at the user-specified frequency for the green LED.
    g_led_state->green_toggles++;
}

// INTERRUPT HANDLER for green LED
ISR(TIMER3_COMPA_vect) {
    // This the Interrupt Service Routine for Toggling the yellow LED.
    // Each time the TCNT count is equal to the OCRxx register, this interrupt is enabled.
    // At creation of this file, it was initialized to interrupt every 100ms (10Hz).
    //
    // Increment ticks. If time, toggle YELLOW and increment toggle counter.
    g_led_state->yellow_toggles++;
    LED_TOGGLE(YELLOW);
}
