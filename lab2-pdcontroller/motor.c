#include <pololu/orangutan.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "log.h"
#include "motor.h"
#include "cli.h"

/*
 * CONSTANTS
 */
#define DEFAULT_MOTOR_SPEED          (100)
#define NUMBER_DARK_REGIONS          (32)
#define NUMBER_TRANSTIONS_REVOLUTION (NUMBER_DARK_REGIONS * 2)
#define MAX_TORQUE                   (255)
#define COEFFICIENT_SCALAR           (1000)

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
    .last_position_change_ms = 0,
    .proportional_gain = (3 * COEFFICIENT_SCALAR),
    .derivative_gain = (1 * COEFFICIENT_SCALAR),
    .last_torque = 0,
    .logging_enabled = false,
    .poll_rate = SERVICE_RATE_50HZ
};

/* Prototypes */
void motor_service_pd_controller(void);

/*
 * Usage: r <degrees:int>
 */
static int clicmd_set_reference(char const * const args)
{
    int32_t target_degrees;
    if (1 == sscanf(args, "%ld", &target_degrees)) {
        g_motor_state.target_position = (int32_t)target_degrees;
    }
    return 0;
}

/*
 * Usage: r+ <degrees:int>
 */
static int clicmd_increase_reference(char const * const args)
{
    int32_t degrees_delta;
    if (1 == sscanf(args, "%ld", &degrees_delta)) {
        g_motor_state.target_position += degrees_delta;
    }
    return 0;
}

/*
 * Usage: r- <degrees:int>
 */
static int clicmd_decrease_reference(char const * const args)
{
    int32_t degrees_delta;
    if (1 == sscanf(args, "%ld", &degrees_delta)) {
        g_motor_state.target_position -= degrees_delta;
    }
    return 0;
}

/*
 * Usage: l
 */
static int clicmd_toggle_logging(char const * const args)
{
    (void)(args);
    g_motor_state.logging_enabled = g_motor_state.logging_enabled ? false : true;
    return 0;
}

/*
 * Usage: v
 */
static int clicmd_view_parameters(char const * const args)
{
    (void)(args);
    LOG("Kd=%d, Kp=%d, ",
            g_motor_state.derivative_gain,
            g_motor_state.proportional_gain);
    LOG("Vm=%ld, Pr=%ld, Pm=%ld, T=%d\r\n",
            g_motor_state.current_velocity,
            motor_get_target_pos(),
            motor_get_current_pos(),
            g_motor_state.last_torque);
    return 0;
}

/*
 * Usage: p <int:Kp value>
 */
static int clicmd_set_kp(char const * const args)
{
    int32_t kp;
    if (1 == sscanf(args, "%ld", &kp)) {
        g_motor_state.proportional_gain = kp;
    }
    return 0;
}

/*
 * Usage: p+ <int:Kp delta>
 */
static int clicmd_increase_kp(char const * const args)
{
    int32_t kp_delta;
    if (1 == sscanf(args, "%ld", &kp_delta)) {
        g_motor_state.proportional_gain += kp_delta;
    }
    return 0;
}

/*
 * Usage: p- <int:Kp delta>
 */
static int clicmd_decrease_kp(char const * const args)
{
    int32_t kp_delta;
    if (1 == sscanf(args, "%ld", &kp_delta)) {
        g_motor_state.proportional_gain -= kp_delta;
    }
    return 0;
}

/*
 * Usage: d <int:Kd value>
 */
static int clicmd_set_kd(char const * const args)
{
    int32_t kd;
    if (1 == sscanf(args, "%ld", &kd)) {
        g_motor_state.derivative_gain = kd;
    }
    return 0;
}

/*
 * Usage: d+ <int:Kd delta>
 */
static int clicmd_increase_kd(char const * const args)
{
    int32_t kd_delta;
    if (1 == sscanf(args, "%ld", &kd_delta)) {
        g_motor_state.derivative_gain += kd_delta;
    }
    return 0;
}

/*
 * Usage: d- <int:Kd delta>
 */
static int clicmd_decrease_kd(char const * const args)
{
    int32_t kd_delta;
    if (1 == sscanf(args, "%ld", &kd_delta)) {
        g_motor_state.derivative_gain -= kd_delta;
    }
    return 0;
}

/*
 * Driver the motor at the specified torque value
 *
 * The desired direction is indicated by the sign of torque
 * and the value should be in the range {-255, 255}.  It is
 * assumed that the motor is attached to the module's
 * motor2 port which uses the following registers:
 *  - OC2B: PWM Signal
 *  - PC6:  Direction
 *
 * We will use fast PWM mode (bit easier to work with).  To
 * change our duty cycle, we change the top value for the
 * output compare match.
 */
static void motor_set_output(int16_t torque)
{
	uint8_t abs_torque = abs(torque);
	// set direction
	if (torque < 0) {
		PORTC &= ~(1 << PC6);
	} else {
		PORTC |= (1 << PC6);
	}
	OCR2B = abs_torque;
}

/*
 * Get the target motor position
 */
int32_t motor_get_target_pos(void)
{
    return g_motor_state.target_position;
}

/*
 * Get the current most position
 */
int32_t motor_get_current_pos(void)
{
    return g_motor_state.current_position;
}

/*
 * Get the last motor torque that was used
 */
int motor_get_last_torque(void)
{
    return g_motor_state.last_torque;
}

/*
 * Log the motor state (if enabled)
 */
void motor_log_state(void)
{
    if (g_motor_state.logging_enabled) {
        LOG("%ld,%ld,%d\r\n",
            g_motor_state.target_position,
            g_motor_state.current_position,
            g_motor_state.last_torque);
    }
}

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

    /* Setup PC6 (direction) and PD6 (PWM) as outputs */
    DDRC |= (1 << PC6);
    DDRD |= (1 << PD6);

    /* We are using Fast PWM for the mode and we want to be able
     * put a PWM signal on OC2B at some frequency */

    /* COM2A1 = 0, COM2A0 = 0; OC2A disconnected (PB3) */
    TCCR2A &= ~(1 << COM2A1 | 1 << COM2A0);

    /* Clear OC2B on Compare Match, set OC2B at BOTTOM, (non-inverting mode). */
    TCCR2A |=  (1 << COM2B1);
    TCCR2A &= ~(1 << COM2B0);

    /* Waveform Generation Mode: Fast PWM
     *
     * TOP = 0xFF, Update of OCRx at BOTTOM, TOV Flag Set on MAX
     */
    TCCR2A |=  (1 << WGM21 | 1 << WGM20);
    TCCR2B &= ~(1 << WGM22);

    /* Clock Select: prescaler of 64 */
    TCCR2B &= ~(1 << CS21 | 1 << CS20);
    TCCR2B |=  (1 << CS22);

    /* Start out by not driving the motor */
    OCR2B = 0;

    (void)(clicmd_toggle_logging);
    (void)(clicmd_view_parameters);
    (void)(clicmd_set_reference);
    (void)(clicmd_increase_reference);

    /* register our CLI command */
    CLI_REGISTER(
        {"l", "toggle logging(enable/disable)",
         clicmd_toggle_logging},
        {"v", "View the current values of Kd, Kp, Vm, Pr, Pm, and T",
         clicmd_view_parameters},
        {"r", "r <degrees>: Set the reference position to degrees",
         clicmd_set_reference},
        {"r+", "r+ <degrees>: Increase reference position by some relative amount",
         clicmd_increase_reference},
        {"r-", "r- <degrees>: Decrease reference position by some relative amount",
         clicmd_decrease_reference},
        {"p", "p <degrees>: Set Kp to the specified value",
         clicmd_set_kp},
        {"p+", "p+ <degrees>: Increase Kp by the specified amount",
         clicmd_increase_kp},
        {"p-", "p- <degrees>: Decrease Kp by the specified amount",
         clicmd_decrease_kp},
        {"d", "d <degrees>: Set Kd to the specified value",
         clicmd_set_kd},
        {"d+", "d+ <degrees>: Increase Kd by the specified amount",
         clicmd_increase_kd},
        {"d-", "d- <degrees>: Decrease Kd by the specified amount",
         clicmd_decrease_kd}
    );
}

void motor_service_pd_controller_5hz(void)
{
    if (g_motor_state.poll_rate == SERVICE_RATE_5HZ) {
        motor_service_pd_controller();
    }
}

void motor_service_pd_controller_50hz(void)
{
    if (g_motor_state.poll_rate == SERVICE_RATE_50HZ) {
        motor_service_pd_controller();
    }
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
    int32_t position_delta, torque;
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
    torque =
        (g_motor_state.proportional_gain * (target_position - current_position) / COEFFICIENT_SCALAR) -
        (g_motor_state.derivative_gain * current_velocity / COEFFICIENT_SCALAR);
    /* update the output value */
    g_motor_state.last_torque = MAX(MIN(torque, MAX_TORQUE), -MAX_TORQUE);
    motor_set_output(g_motor_state.last_torque);
}
