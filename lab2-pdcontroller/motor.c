#include <pololu/orangutan.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "log.h"
#include "motor.h"

/*
 * CONSTANTS
 */
#define DEFAULT_MOTOR_SPEED          (100)
#define NUMBER_DARK_REGIONS          (32)
#define NUMBER_TRANSTIONS_REVOLUTION (NUMBER_DARK_REGIONS * 2)
#define MAX_TORQUE                   (255)

/* Kp - The 'P' in PD */
#define PROPORTIONAL_GAIN (1)
/* Kd  - The 'D' in PD */
#define DERIVATIVE_GAIN   (1)

/*
 * MACROS
 */
#define MIN(a, b)     (a < b ? a : b)
#define MAX(a, b)     (a > b ? a : b)

/* Globals */
static timers_state_t * g_timers_state;
static motor_state_t g_motor_state = {
    .current_torque = 0,
    .current_position = 0,
    .target_position = 0,
    .current_velocity = 0,
    .last_position_change_ms = 0
};

/*
 * MOTOR FUNCTIONS
 */
void motor_init(timers_state_t * timers_state)
{
    g_timers_state = timers_state;

    /* setup encoder (only 1 motor - motor 2) */
    encoders_init(IO_D3, IO_D2, IO_D1, IO_D0);

    /* reset counters */
    encoders_get_counts_and_reset_m2();
}

/*
 * The PD controller maintains the position of the motor using both the
 * reference and measured position of the motor. It is of the highest
 * priority in the system, and it should run at a frequency of 1KHz. The
 * controller that you will implement calculates a desired torque
 * according to the equation
 *
 *     T = Kp(Pr - Pm) - Kd*Vm
 *
 * where
 *
 *     T = Output motor signal (torque)
 *     Pr = Desired motor position
 *     Pm = Current motor position
 *     Vm = Current motor velocity (computed based on finite differences)
 *     Kp = Proportional gain
 *     Kd = Derivative gain
 *
 * with better names,
 *
 *     Torque = (proportional_gain * (target_position - current_position) -
 *               derivative_gain * current_velocity)
 *
 * T is a signal that can be used directly to control the motor, except
 * check that it is in range { -TOP, TOP }, and use absolute(T) and set
 * motor direction appropriately.
 *
 * Pm and Vm can be computed using the encoder attached to the motor. The
 * encoder generates a pair of signals as the motor shaft rotates, which
 * are used as input signals into 2 general I/O port pins on the
 * Orangutan. Using the Orangutan libraries (the wheel encoder example)
 * or your own code, set up pin change interrupts on these port pins and
 * count the number of signal changes to track the position of the motor.
 *
 * Pr is provided through the user interface or as part of a hard-coded
 * trajectory.
 *
 * Kp and Kd are the terms that you define that determine how your system
 * behaves. You will need to determine these values experimentally. See
 * discussion below under trajectory interpolator to figure out where to
 * start with these values. You should always be generating motor
 * commands, regardless of whether the reference position is changing or
 * not. This means that at any time, if you move the wheel manually, then
 * the PD controller should bring it back to the current reference
 * position. In other words, even if the motor is where it should be, do
 * not stop sending commands to the motor, instead send it 0 (or whatever
 * torque value your controller produces).
 */
void motor_service_pd_controller(void)
{
    int32_t position_delta;
    int encoder_count = encoders_get_counts_m2();
    int32_t current_position = (int32_t)(360 * ((double)encoder_count / NUMBER_TRANSTIONS_REVOLUTION));
    uint32_t now_ms = g_timers_state->ms_ticks;
    if ((position_delta = current_position - g_motor_state.current_position) != 0) {
        int32_t time_delta = now_ms - g_motor_state.last_position_change_ms;
        g_motor_state.current_velocity = position_delta / time_delta;
        g_motor_state.current_position = current_position;
    }

    int32_t target_position = g_motor_state.target_position;
    int current_velocity = g_motor_state.current_velocity;
    int torque =
        (PROPORTIONAL_GAIN * (target_position - current_position) -
                DERIVATIVE_GAIN * current_velocity);
    /* update the output value */
    set_motors(0, torque);
}
