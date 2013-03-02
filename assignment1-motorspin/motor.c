/*
 * 
 */
#include <pololu/orangutan.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* something medium speed */
#define MOTOR_SPEED                  (100)
/* based on my count */
#define NUMBER_DARK_REGIONS          (32)
/* will enter and exit every dark region once per revolution */
#define NUMBER_TRANSTIONS_REVOLUTION (NUMBER_DARK_REGIONS * 2)

typedef enum {
  DIRECTION_REVERSE = -1,
  DIRECTION_FORWARD = 1
} direction_e;

typedef enum {
  BUTTON_STATE_PRESSED,
  BUTTON_STATE_RELEASED
} button_state_e;

typedef struct {
  /* the last d0 reading */
  uint8_t d0_value;
  /* the last d1 reading */
  uint8_t d1_value;
  /* the number of transitions (1 -> 0 or 0 -> 1) */
  uint8_t number_transitions;
  /* direction (forward/revrese) for the motor */
  direction_e direction;
  /* is the motor function enabled? */
  bool enabled;
} motor_state_t;

typedef struct {
  button_state_e state;
} button_state_t;

/* Globals */
static motor_state_t g_motor_state;
static button_state_t g_middle_button;
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
  g_motor_state.enabled = true;
}

static void motor_readState()
{
  uint8_t new_d0_value, new_d1_value;
  new_d0_value = is_digital_input_high(IO_D0) ? 1 : 0;
  new_d1_value = is_digital_input_high(IO_D1) ? 1 : 0;

  /* check for transitions... for now just look at D0 */
  if (new_d1_value != g_motor_state.d1_value) {
    if (++g_motor_state.number_transitions > (2 * NUMBER_TRANSTIONS_REVOLUTION)) {
      g_motor_state.number_transitions = 0;
      g_motor_state.direction = g_motor_state.direction * -1; // invert
    }
  }
  /* record current state */
  g_motor_state.d0_value = new_d0_value;
  g_motor_state.d1_value = new_d1_value;
}

static void motor_drive() {
  if (g_motor_state.enabled) {
    set_motors(0, g_motor_state.direction * MOTOR_SPEED);
  } else {
    set_motors(0, 0);
  }
}

/*
 * BUTTON FUNCTIONS
 */
static void button_init() {
  g_middle_button.state = BUTTON_STATE_RELEASED;
}

static void button_readState() {
  switch (g_middle_button.state) {
  case BUTTON_STATE_RELEASED:
    if (get_single_debounced_button_press(MIDDLE_BUTTON)) {
      g_middle_button.state = BUTTON_STATE_PRESSED;
      if (g_motor_state.enabled) {
	g_motor_state.enabled = false;
	g_motor_state.number_transitions = 0;
	g_motor_state.direction = DIRECTION_FORWARD;
      } else {
	g_motor_state.enabled = true;
      }
    }
    break;
  case BUTTON_STATE_PRESSED:
    g_middle_button.state = BUTTON_STATE_RELEASED;
    break;
  }
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
