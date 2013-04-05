#ifndef MOTOR_H_
#define MOTOR_H_

#include <inttypes.h>
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
} motor_state_t;

void motor_init(timers_state_t * timers_state);
void motor_service_pd_controller(void);
void motor_drive(void);

#endif /* MOTOR_H_ */
