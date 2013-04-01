/*******************************************
*
* Header file for menu stuff.
*
*******************************************/
#ifndef __MENU_H
#define __MENU_H

#include <inttypes.h> //gives us uintX_t
#include "leds.h"
#include "timers.h"

int menu_init(led_state_t * led_state, timers_state_t * timers_state);
int menu_service(void);

#endif //__MENU_H
