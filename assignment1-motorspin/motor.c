/*
 * 
 */
#include <pololu/orangutan.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* something medium speed */
#define DEFAULT_MOTOR_SPEED          (100)
/* based on my count */
#define NUMBER_DARK_REGIONS          (32)
/* will enter and exit every dark region once per revolution */
#define NUMBER_TRANSTIONS_REVOLUTION (NUMBER_DARK_REGIONS * 2)

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

typedef enum {
  DIRECTION_REVERSE = -1,
  DIRECTION_FORWARD = 1
} direction_e;

typedef enum {
  BUTTON_STATE_PRESSED,
  BUTTON_STATE_RELEASED
} button_state_e;

typedef struct {
  /* direction (forward/revrese) for the motor */
  direction_e direction;
  /* is the motor function enabled? */
  bool enabled;
  /* speed to drive the motor (with direction) */
  uint8_t speed;
} motor_state_t;

typedef struct {
  button_state_e state;
} button_state_t;

/* Globals */
static motor_state_t g_motor_state;
static button_state_t g_middle_button;
static button_state_t g_top_button;
static button_state_t g_bottom_button;
static int g_tick;

/*
 * MOTOR FUNCTIONS
 */
static void motor_init()
{
  /* setup encoder (only 1 motor - motor 2) */
  encoders_init(IO_D3, IO_D2, IO_D1, IO_D0);

  /* initial data values */
  encoders_get_counts_and_reset_m2();
  g_motor_state.direction = DIRECTION_FORWARD;
  g_motor_state.enabled = true;
  g_motor_state.speed = DEFAULT_MOTOR_SPEED;
}

static void motor_serviceEncoders()
{
  /* check for transitions... for now just look at D0 */
  int encoder_count = encoders_get_counts_m2();
  if (encoder_count > (2 * NUMBER_TRANSTIONS_REVOLUTION)) {
    g_motor_state.direction = g_motor_state.direction * -1; // invert
    encoders_get_counts_and_reset_m2();
  } else if (encoder_count < (-2 * NUMBER_TRANSTIONS_REVOLUTION)) {
    g_motor_state.direction = g_motor_state.direction * -1; // invert
    encoders_get_counts_and_reset_m2();
  }
  if (!g_motor_state.enabled) {
    encoders_get_counts_and_reset_m2();
  }
}

static void motor_drive() {
  if (g_motor_state.enabled) {
    set_motors(0, g_motor_state.direction * g_motor_state.speed);
  } else {
    set_motors(0, 0);
  }
}

/*
 * BUTTON FUNCTIONS
 */
static void button_init() {
  g_middle_button.state = BUTTON_STATE_RELEASED;
  g_top_button.state = BUTTON_STATE_RELEASED;
  g_bottom_button.state = BUTTON_STATE_RELEASED;
}

static void button_readState() {
  /* middle button - pause */
  int button_press;
  int button_release;
  button_press = get_single_debounced_button_press(ANY_BUTTON);
  button_release = get_single_debounced_button_release(ANY_BUTTON);
  switch (g_middle_button.state) {
  case BUTTON_STATE_RELEASED:
    if (button_press & MIDDLE_BUTTON) {
      g_middle_button.state = BUTTON_STATE_PRESSED;
      if (g_motor_state.enabled) {
	g_motor_state.enabled = false;
	g_motor_state.direction = DIRECTION_FORWARD;
	play_note(A(5), 200, 12);
      } else {
	g_motor_state.enabled = true;
	play_note(A(5), 200, 12);
      }
    }
    break;
  case BUTTON_STATE_PRESSED:
    if (button_release & MIDDLE_BUTTON) {
      g_middle_button.state = BUTTON_STATE_RELEASED;
    }
    break;
  }

  /* bottom button - speed down */
  switch (g_bottom_button.state) {
  case BUTTON_STATE_RELEASED:
    if (button_press & BOTTOM_BUTTON) {
      g_bottom_button.state = BUTTON_STATE_PRESSED;
      g_motor_state.speed = MIN(g_motor_state.speed + 5, 255);
    }
    break;
  case BUTTON_STATE_PRESSED:
    if (button_release & BOTTOM_BUTTON) {
      g_bottom_button.state = BUTTON_STATE_RELEASED;
    }
    break;
  }

  /* top button - speed up */
  switch (g_top_button.state) {
  case BUTTON_STATE_RELEASED:
    if (button_press & TOP_BUTTON) {
      g_top_button.state = BUTTON_STATE_PRESSED;
      g_motor_state.speed = MAX(g_motor_state.speed - 5, 0);
    }
    break;
  case BUTTON_STATE_PRESSED:
    if (button_release & TOP_BUTTON) {
      g_top_button.state = BUTTON_STATE_RELEASED;
    }
    break;
  }
  
}

/*
 * PRINT FUNCTIONS
 */
static void print_service() {
  char buf[128];
  if (g_tick % 2000 == 0) {
    clear();

    /* display state (for/rev/pau) */
    lcd_goto_xy(0, 0);
    sprintf(buf, "[%s]",
	    g_motor_state.enabled ? (g_motor_state.direction == DIRECTION_FORWARD ? "fwd" : "rev") : "pau");
    print(buf);

    /* display speed */
    lcd_goto_xy(7, 0);
    sprintf(buf, "spd=%u", g_motor_state.speed);
    print(buf);

    /* display encoder count */
    lcd_goto_xy(0, 1);  // second row
    sprintf(buf, "Count: %d", encoders_get_counts_m2());
    print(buf);
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
    motor_serviceEncoders();
    motor_drive();
    button_readState();
    print_service();
  }
}
