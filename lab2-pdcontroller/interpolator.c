#include <stdint.h>
#include "interpolator.h"
#include "timers.h"
#include "deque.h"

#define NUMBER_DARK_REGIONS          (32)
#define NUMBER_TRANSTIONS_REVOLUTION (NUMBER_DARK_REGIONS * 2)
#define ENDZONE_MS                   (1000)

typedef struct {
	int32_t position;
} interpolator_target_t;
static uint8_t number_targets = 0;
static interpolator_target_t targets[10];
static int32_t time_entered_end_zone = -1;

/* The current position in degrees (based on encoder) */
int32_t current_position;
/* The target position in degrees */
int32_t target_position;
/* The current estimate velocity in degrees/second */
int32_t current_velocity;
/* for tracking velocity */
uint32_t last_position_change_ms;

static void
interpolator_pop_target_position(void)
{
	if (number_targets > 0) {
		number_targets--;
	}
}

/*
 * Initialize the linear interpolator
 */
void
interpolator_init(void) {}

/*
 * Service the interpolator (get the updated encoder counts, etc.)
 */
void
interpolator_service(void)
{
	if (number_targets > 0) {
		if (abs(interpolator_get_target_position() - interpolator_get_current_position()) < 15) {
			if (time_entered_end_zone < 0) {
				time_entered_end_zone = timers_get_uptime_ms();
			}
			if (timers_get_uptime_ms() - time_entered_end_zone > ENDZONE_MS) {
				time_entered_end_zone = -1;
				interpolator_pop_target_position();
			}
		}
	}
}

/*
 * Get the current position in degrees (from encoders)
 */
int32_t
interpolator_get_current_position(void)
{
	int encoder_count = encoders_get_counts_m2();
	int32_t current_position = (int32_t)(360 * ((double)encoder_count / NUMBER_TRANSTIONS_REVOLUTION));
	return current_position;
}

/*
 * Add a target position to the interpolation plan
 *
 * The interpolator will target this immediately if it is not already
 * targetting a position.  The interpolator will target the next position
 * if it has been within 15 degrees of its target for 1s.
 */
void
interpolator_add_target_position(int32_t target_position)
{
	if (number_targets < COUNT_OF(targets)) {
		targets[number_targets++].position = target_position;
	}
}

/*
 * Get the current interpolator adjusted target position (absolute)
 */
int32_t
interpolator_get_target_position(void)
{
	// NOTE: this could be the last "done" one at index 0, this is OK
	return targets[number_targets].position;
}
