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

  /* period information */
  volatile uint16_t red_perdiod;
  volatile uint16_t green_period;
  volatile uint16_t yellow_period;
} led_state_t;

// define the data direction registers
#define DD_REG_RED	      (DDRD)
#define DD_REG_YELLOW     (DDRD)
#define DD_REG_GREEN      (DDRD)

// define the output ports by which you send signals to the LEDs
#define PORT_RED     (PORTD)
#define PORT_YELLOW  (PORTD)
#define PORT_GREEN   (PORTC)

// define the bit-masks for each port that the LEDs are attached to
#define BIT_RED      (1 << 1)
#define BIT_YELLOW   (1 << 0)
#define BIT_GREEN    (1 << 4)

// define "function" calls for turning LEDs on and off
#define LED_ON(x)     (PORT_##x |= BIT_##x)
#define LED_OFF(x)    (PORT_##x &= ~(BIT_##x))
#define LED_TOGGLE(x) (PORT_##x ^= BIT_##x)

// function call prototypes
void leds_init();
void leds_set_toggle(char color, int ms);

#endif

