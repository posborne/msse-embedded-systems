/*
 * interpolator.h
 */

#ifndef INTERPOLATOR_H_
#define INTERPOLATOR_H_

#include <stdint.h>
#include "timers.h"

#define VELOCITY_POLL_MS (50)

int32_t interpolator_get_current_position(void);
int32_t interpolator_get_target_position(void);
int32_t interpolator_get_current_velocity(void);
int32_t interpolator_get_absolute_target_position(void);
void interpolator_add_target_position(int32_t target_position);
void interpolator_init(timers_state_t * timers_state);
void interpolator_service(void);
void interpolator_add_relative_target(int32_t degrees_delta);
void interpolator_service_calc_velocity(void);

#endif /* INTERPOLATOR_H_ */
