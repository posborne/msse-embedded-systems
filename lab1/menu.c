#include "menu.h"
#include "usart.h"
#include "LEDs.h"
#include "timer.h"

#include <stdio.h>
#include <inttypes.h>

// GLOBALS
extern uint32_t G_red_toggles;
extern uint32_t G_green_toggles;
extern uint32_t G_yellow_toggles;

void init_menu() {
	// nothing to do really.
	printf("MENU: {T/P/Z} {R/G/Y/A} <int>\n");
	printf("> ");
}

// Parse user-input and implement commands
void menu() {

	char color;
	char op_char;
	uint8_t* input;
	int parsed;
	int value;

		input = USART1_get_input();
		parsed = sscanf(input, "%c %c %d", &op_char, &color, &value);
		printf("Parsed as op:%c color:%c value:%d\n", op_char, color, value);
		
		/* convert color to upper and check if valid*/
		color -= 32*(color>='a' && color<='z');
		switch (color) {
			case 'R':
			case 'G':
			case 'Y': 
			case 'A': break;
			default:
				printf("Bad color\n");
				return;
		}

		/* Check valid command and implement */
		switch (op_char) {

			/* change toggle frequencey for <color> LED */
			case 'T':
			case 't':
				set_toggle(color,value);
				break;

			/* print counter for <color> LED */
			case 'P':
			case 'p':
				switch(color) {
					case 'R': printf("R toggles: %d\n",G_red_toggles); break;
					case 'G': printf("G toggles: %d\n",G_green_toggles); break;
					case 'Y': printf("Y toggles: %d\n",G_yellow_toggles); break;
					case 'A': printf("toggles R:%d  G:%d  Y:%d\n",(int)G_red_toggles,(int)G_green_toggles,(int)G_yellow_toggles); break;
					default: printf("How did you get here too??\n");
				}
				break;

			/* zero counter for <color> LED */
			case 'Z':
			case 'z':
				switch(color) {
					case 'R': G_red_toggles=0; break;
					case 'G': G_green_toggles=0; break;
					case 'Y': G_yellow_toggles=0; break;
					case 'A': G_red_toggles = G_green_toggles = G_yellow_toggles = 0; break;
					default: printf("How did you get here 3??\n");
				}
				break;
			default:
				printf("%s does not compute??\n",input);

		} // end switch(op_char) 
	printf("> ");
} // end menu()


