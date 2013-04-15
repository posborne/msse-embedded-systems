/*
 * interpolator.h
 */

#ifndef INTERPOLATOR_H_
#define INTERPOLATOR_H_

#include <stdint.h>

int32_t interpolator_get_current_position(void);
int32_t interpolator_get_target_position(void);
void interpolator_add_target_position(int32_t target_position);
void interpolator_init(void);
void interpolator_service(void);

#endif /* INTERPOLATOR_H_ */
