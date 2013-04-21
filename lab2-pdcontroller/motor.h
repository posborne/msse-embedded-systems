#ifndef MOTOR_H_
#define MOTOR_H_

#include <inttypes.h>
#include <stdbool.h>
#include "timers.h"

typedef enum {
    SERVICE_RATE_5HZ,
    SERVICE_RATE_50HZ
} pd_controller_poll_state_e;

typedef struct {
    /* The last torque value used to drive the motor */
    uint8_t current_torque;
    /* Kp - The 'P' in PD (0.01/bit) */
    int32_t proportional_gain;
    /* Kp - The 'P' in PD (0.01/bit) */
    int32_t derivative_gain;
    /* last torque value */
    int last_torque;
    /* logging enabled/disabled */
    bool logging_enabled;
    /* poll rate */
    pd_controller_poll_state_e poll_rate;
} motor_state_t;

void motor_init(timers_state_t * timers_state);
void motor_service_pd_controller_5hz(void);
void motor_service_pd_controller_50hz(void);
void motor_drive(void);
int32_t motor_get_target_pos(void);
int32_t motor_get_current_pos(void);
int motor_get_last_torque(void);
void motor_log_state(void);

#endif /* MOTOR_H_ */
