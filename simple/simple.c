#include <pololu/orangutan.h>

#define DELAY (250)

int main()
{
  while (1) {
    set_digital_output(IO_C1, 1);
    red_led(1);
    delay_ms(DELAY);
    set_digital_output(IO_C1, 0);
    red_led(0);
    delay_ms(DELAY);
    set_digital_output(IO_C1, 1);
    green_led(1);
    delay_ms(DELAY);
    set_digital_output(IO_C1, 0);
    green_led(0);
    delay_ms(DELAY);
  }
  return 0;
}
