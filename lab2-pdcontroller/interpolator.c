#include <stdint.h>
#include <stdlib.h>
#include <pololu/orangutan.h>
#include "interpolator.h"
#include "timers.h"
#include "deque.h"
#include "macros.h"
#include "log.h"

#define NUMBER_DARK_REGIONS          (32)
#define NUMBER_TRANSTIONS_REVOLUTION (NUMBER_DARK_REGIONS * 2)
#define ENDZONE_MS                   (1000)
#define MAX_DELTA                    (90)
#define CLOSE_ENOUGH_DEGREES         (15)

typedef struct {
	int32_t position;
} interpolator_target_t;
static uint8_t number_targets = 0;
static interpolator_target_t targets[10];
static int32_t time_entered_end_zone = -1;

/* The current position in degrees (based on encoder) */
int32_t current_position;
/* The current estimate velocity in degrees/second */
int32_t current_velocity;
/* for tracking velocity */
int32_t last_position;
/* store a ref to the current target */
interpolator_target_t * current_target = &targets[0];

static void
interpolator_pop_target_position(void)
{
	if (number_targets > 0) {
		number_targets--;
		current_target = &targets[number_targets];
	}
}

/*
 * Get a ptr to the current or NULL
 */
static interpolator_target_t *
interpolator_get_current_target(void)
{
    if (number_targets > 0) {
        return &targets[number_targets];
    }
    return NULL;
}

/*
 * Initialize the linear interpolator
 */
void
interpolator_init(void)
{
    /* warm up the velocity calculation service */
    last_position = interpolator_get_current_position();
    current_velocity = 0;
}

/*
 * Update the velocity calculation
 */
void interpolator_service_calc_velocity(void)
{
    /* TODO: normalize with respect to VELOCITY_POLL_MS */
    int32_t new_position = interpolator_get_current_position();
    current_velocity = new_position - last_position;
    last_position = new_position;
}

/*
 * Service the interpolator (get the updated encoder counts, etc.)
 */
void
interpolator_service(void)
{
	if (number_targets > 0) {
		if (abs(interpolator_get_target_position() - interpolator_get_current_position()) < CLOSE_ENOUGH_DEGREES) {
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
    interpolator_target_t * t = interpolator_get_current_target();
    if (t != NULL) {
        int32_t delta = t->position - interpolator_get_current_position();
        if (delta > MAX_DELTA) {
            return interpolator_get_current_position() - MAX_DELTA;
        } else if (delta < -MAX_DELTA) {
            return interpolator_get_current_position() + MAX_DELTA;
        } else {
            return t->position;
        }
    } else {
        return interpolator_get_current_position();
    }
}

/*
 * Get the current velocity of the motor
 */
int32_t
interpolator_get_current_velocity(void)
{
    return current_velocity;
}

/*
 * Get the absolute (not limited) target position in degrees
 */
int32_t
interpolator_get_absolute_target_position(void)
{
    interpolator_target_t * t = interpolator_get_current_target();
    if (t != NULL) {
        return t->position;
    } else {
        return interpolator_get_current_position();
    }
}

/*
 * Add a new target that is some delta from the last registered target
 *
 * That is, if I have 3 target destinations in the pipeline [0, 1080, 900]
 * and I add a relative target of -360, a fourth target of 720 will be added.
 */
void
interpolator_add_relative_target(int32_t degrees_delta)
{
    /* get the last target if any, default to current position */
    interpolator_target_t * t = interpolator_get_current_target();
    if (t != NULL) {
        return interpolator_add_target_position(t->position + degrees_delta);
    } else {
        return interpolator_add_target_position(interpolator_get_current_position() + degrees_delta);
    }
}

