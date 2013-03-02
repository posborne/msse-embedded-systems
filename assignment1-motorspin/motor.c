/*
 * 
 */
#include <pololu/orangutan.h>
#include <stdint.h>
#include <stdio.h>

#define MOTOR_SPEED                  (100)
#define NUMBER_DARK_REGIONS          (32)
#define NUMBER_TRANSTIONS_ROTATION   (NUMBER_DARK_REGIONS * 2)

typedef enum {
  DIRECTION_REVERSE = -1,
  DIRECTION_FORWARD = 1
} direction_t;

typedef struct {
  /* the last d0 reading */
  uint8_t d0_value;
  /* the last d1 reading */
  uint8_t d1_value;
  /* the number of transitions (1 -> 0 or 0 -> 1) */
  uint8_t number_transitions;
  /* direction (forward/revrese) for the motor */
  direction_t direction;
} motor_state_t;

/* Globals */
static motor_state_t g_motor_state;
static int g_tick;

/*
 * MOTOR FUNCTIONS
 */
static void motor_init()
{
  /* setup pins as inputs */
  set_digital_input(IO_D0, PULL_UP_ENABLED);
  set_digital_input(IO_D1, PULL_UP_ENABLED);

  /* initial data reading */
  g_motor_state.number_transitions = 0;
  g_motor_state.d0_value = is_digital_input_high(IO_D0) ? 1 : 0;
  g_motor_state.d1_value = is_digital_input_high(IO_D1) ? 1 : 0;
  g_motor_state.direction = DIRECTION_FORWARD;
}

static void motor_readState()
{
  uint8_t new_d0_value, new_d1_value;
  new_d0_value = is_digital_input_high(IO_D0) ? 1 : 0;
  new_d1_value = is_digital_input_high(IO_D1) ? 1 : 0;
  
  /* check for transitions... for now just look at D0 */
  if (new_d0_value != g_motor_state.d0_value) {
    if (++g_motor_state.number_transitions > (2 * NUMBER_TRANSTIONS_ROTATION)) {
      g_motor_state.number_transitions = 0;
      g_motor_state.direction = g_motor_state.direction * -1; // invert
    }
  }
  /* record current state */
  g_motor_state.d0_value = new_d0_value;
  g_motor_state.d1_value = new_d1_value;
}

static void motor_drive() {
  set_motors(0, g_motor_state.direction * MOTOR_SPEED);  
}

/*
 * BUTTON FUNCTIONS
 */
static void button_init() {
}

static void button_readState() {
}


/*
 * PRINT FUNCTIONS
 */
static void print_service() {
  char buf1[128];
  char buf2[128];
  if (g_tick % 1000 == 0) {
    clear();
    sprintf(buf1, "D0: %u, D1: %u",
	    g_motor_state.d0_value,
	    g_motor_state.d1_value);
    print(buf1);
    lcd_goto_xy(0, 1);  // second row
    sprintf(buf2, "Count: %u",
	    g_motor_state.number_transitions);
    print(buf2);
  }
}

/*
 * Main Loop
 */
int main()
{
  motor_init();
  button_init();
  g_tick = 0;
  while(1)
  {
    g_tick++; /* note: will roll over */
    motor_readState();
    motor_drive();
    button_readState();
    print_service();
  }
}
