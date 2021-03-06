#ifndef __LEDs_H
#define __LEDs_H

#include <avr/io.h>
#include <stdint.h>

/* keep tools happy, worried about undefined symbols */
#define RED    ()
#define GREEN  ()
#define YELLOW ()

typedef struct {
  /* toggle counts */
  volatile uint32_t red_toggles;
  volatile uint32_t green_toggles;
  volatile uint32_t yellow_toggles;
} led_state_t;

// define the data direction registers
#define DD_REG_GREEN    (DDRD)
#define DD_REG_YELLOW   (DDRA)
#define DD_REG_RED	    (DDRA)

// define the output ports by which you send signals to the LEDs
#define PORT_GREEN     (PORTD)
#define PORT_YELLOW    (PORTA)
#define PORT_RED       (PORTA)

// define the bit-masks for each port that the LEDs are attached to
#define BIT_GREEN      (1 << 5)
#define BIT_YELLOW     (1 << 0)
#define BIT_RED        (1 << 2)

// define "function" calls for turning LEDs on and off
#define LED_DISABLE(x) (DD_REG_##x &= ~(BIT_##x))
#define LED_ENABLE(x)  (DD_REG_##x |= BIT_##x)
#define LED_ENABLED(x) ((DD_REG_##x & BIT_##x) > 0)
#define LED_ON(x)      (PORT_##x |= BIT_##x)
#define LED_OFF(x)     (PORT_##x &= ~(BIT_##x))
#define LED_TOGGLE(x)  (PORT_##x ^= BIT_##x)

// function call prototypes
void leds_init();

#endif

