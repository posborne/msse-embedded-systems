#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "leds.h"
#include "timers.h"

#define DELAY_MS 110
#define GREEN_DELAY_TEST
//#define YELLOW_DELAY_TEST

static led_state_t * g_led_state;

void leds_init(led_state_t * led_state) { 
    g_led_state = led_state;

    // Configure data direction as output for each LED
    LED_ENABLE(RED);
    LED_ENABLE(GREEN);
    LED_ENABLE(YELLOW);
}


/*
 * Interrupt Service Routines
 */

// INTERRUPT HANDLER for yellow LED
ISR(TIMER1_COMPA_vect) {
    // This the Interrupt Service Routine for tracking green toggles. The toggling is done in hardware.
    // Each time the TCNT count is equal to the OCRxx register, this interrupt is enabled.
    // This interrupts at the user-specified frequency for the green LED.
#ifdef YELLOW_DELAY_TEST
    int i;
    for (i = 0; i < (DELAY_MS / 10); i++) {
        WAIT_10MS
    }
#endif
    g_led_state->green_toggles++;
}

// INTERRUPT HANDLER for green LED
ISR(TIMER3_COMPA_vect) {
    // This the Interrupt Service Routine for Toggling the yellow LED.
    // Each time the TCNT count is equal to the OCRxx register, this interrupt is enabled.
    // At creation of this file, it was initialized to interrupt every 100ms (10Hz).
    //
    // Increment ticks. If time, toggle YELLOW and increment toggle counter.
#ifdef GREEN_DELAY_TEST
    int i;
    for (i = 0; i < (DELAY_MS / 10); i++) {
        WAIT_10MS
    }
#endif
    g_led_state->yellow_toggles++;
    LED_TOGGLE(YELLOW);
}
