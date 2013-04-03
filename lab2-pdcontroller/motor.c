#include <pololu/orangutan.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "log.h"

/*
 * CONSTANTS
 */
#define DEFAULT_MOTOR_SPEED          (100)
#define NUMBER_DARK_REGIONS          (32)
#define NUMBER_TRANSTIONS_REVOLUTION (NUMBER_DARK_REGIONS * 2)

/*
 * MACROS
 */
#define MIN(a, b)     (a < b ? a : b)
#define MAX(a, b)     (a > b ? a : b)

/*
 * Type Definitions
 */
typedef enum {
    DIRECTION_REVERSE = -1,
    DIRECTION_FORWARD = 1
} direction_e;

typedef struct {
    direction_e direction; /* direction (forward/revrese) for the motor */
    bool enabled; /* is the motor function enabled? */
    uint8_t speed; /* speed to drive the motor (with direction) */
} motor_state_t;

/* Globals */
static motor_state_t g_motor_state = {
    .direction = DIRECTION_FORWARD,
    .enabled = true,
    .speed = 128
};

/*
 * MOTOR FUNCTIONS
 */
void motor_init(void)
{
    /* setup encoder (only 1 motor - motor 2) */
    encoders_init(IO_D3, IO_D2, IO_D1, IO_D0);

    /* initial data values */
    encoders_get_counts_and_reset_m2();
    g_motor_state.direction = DIRECTION_FORWARD;
    g_motor_state.enabled = true;
    g_motor_state.speed = DEFAULT_MOTOR_SPEED;
}

/*
 * Service the encoders
 */
void motor_service_pd_controller(void)
{
    /* check for transitions... for now just look at D0 */
    int encoder_count = encoders_get_counts_m2();
    LOG("encoder count: %d\r\n", encoder_count);
    if (encoder_count > (2 * NUMBER_TRANSTIONS_REVOLUTION)) {
        LOG("motor: fwd -> reverse");
        g_motor_state.direction = g_motor_state.direction * -1; // invert
        encoders_get_counts_and_reset_m2();
    } else if (encoder_count < (-2 * NUMBER_TRANSTIONS_REVOLUTION)) {
        LOG("motor: reverse -> fwd");
        g_motor_state.direction = g_motor_state.direction * -1; // invert
        encoders_get_counts_and_reset_m2();
    }
    if (!g_motor_state.enabled) {
        encoders_get_counts_and_reset_m2();
    }
}

/*
 * Drive the motor
 */
void motor_drive(void)
{
    if (g_motor_state.enabled) {
        set_motors(0, g_motor_state.direction * g_motor_state.speed);
    } else {
        set_motors(0, 0);
    }
}
