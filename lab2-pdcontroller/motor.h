#ifndef MOTOR_H_
#define MOTOR_H_

#include <inttypes.h>
#include <stdbool.h>
#include "timers.h"

typedef struct {
    /* The last torque value used to drive the motor */
    uint8_t current_torque;
    /* The current position in degrees (based on encoder) */
    int32_t current_position;
    /* The target position in degrees */
    int32_t target_position;
    /* The current estimate velocity in degrees/second */
    int32_t current_velocity;
    /* for tracking velocity */
    uint32_t last_position_change_ms;
    /* Kp - The 'P' in PD (0.01/bit) */
    int16_t proportional_gain;
    /* Kp - The 'P' in PD (0.01/bit) */
    int16_t derivative_gain;
    /* last torque value */
    int last_torque;
    /* logging enabled/disabled */
    bool logging_enabled;
} motor_state_t;

void motor_init(timers_state_t * timers_state);
void motor_service_pd_controller(void);
void motor_drive(void);
int32_t motor_get_target_pos(void);
int32_t motor_get_current_pos(void);
int motor_get_last_torque(void);
void motor_log_state(void);

#endif /* MOTOR_H_ */
