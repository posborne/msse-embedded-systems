#include <pololu/orangutan.h>

/*
 * motors1: for the Orangutan LV, SV, SVP, X2, Baby-O and 3pi robot.
 *
 * This example uses the OrangutanMotors functions to drive
 * motors in response to the position of user trimmer potentiometer
 * and blinks the red user LED at a rate determined by the trimmer
 * potentiometer position.  It uses the OrangutanAnalog library to measure
 * the trimpot position, and it uses the OrangutanLEDs library to provide
 * limited feedback with the red user LED.
 *
 * http://www.pololu.com/docs/0J20
 * http://www.pololu.com
 * http://forum.pololu.com
 */

int main()
{
  int i = -255;
  int going_up = 1;
  while(1)
  {
    if (going_up) {
      if (++i > 255) {
        going_up = 0;
        i = 255;
      }
    } else {
      if (--i < -255) {
        going_up = 1;
        i = -255;
      }
    }
    set_motors(0, i);
    //set_motors(0, 0);
	delay_ms(100);
  }
}
