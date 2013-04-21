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
//#define MAX_DELTA                    (720)
#define CLOSE_ENOUGH_DEGREES         (5)

typedef enum {
    STATE_IN_ENDZONE,
    STATE_OUT_OF_ENDZONE
} interpolator_state_t;

typedef struct _interpolator_target_node_t {
	bool in_use;
    int32_t position;
	struct _interpolator_target_node_t * next;
} interpolator_target_node_t;

/* queue */
static interpolator_target_node_t q_nodes[10];
static interpolator_target_node_t * q_head = NULL;

/* The current position in degrees (based on encoder) */
int32_t current_position;
/* The current estimate velocity in degrees/second */
int32_t current_velocity;
/* for tracking velocity */
int32_t last_position;
/* The time (ms) at which we entered an "endzone" */
static uint16_t time_entered_end_zone = 0;
static interpolator_state_t state = STATE_OUT_OF_ENDZONE;
static timers_state_t * g_timers_state;

static interpolator_target_node_t *
q_alloc_node(void)
{
    int i;
    for (i = 0; i < COUNT_OF(q_nodes); i++) {
        if (!q_nodes[i].in_use) {
            q_nodes[i].in_use = true;
            return &q_nodes[i];
        }
    }
    return NULL;
}

static interpolator_target_node_t *
q_get_tail(void)
{
    /* tail is first node not pointing to another node */
    interpolator_target_node_t * node = q_head;
    if (node == NULL) {
        return NULL;
    }

    while (node->next != NULL) {
        node = node->next;
    }
    return node;
}

//static int
//q_count(void)
//{
//    interpolator_target_node_t * node = q_head;
//    if (q_head == NULL) {
//        return 0;
//    } else {
//        int i = 1;
//        while ((node = node->next) != NULL) {
//            i++;
//        }
//        return i;
//    }
//}

static void
q_append(int32_t position)
{
    interpolator_target_node_t * tail = q_get_tail();
    interpolator_target_node_t * node = q_alloc_node();
    node->next = NULL;
    node->position = position;
    if (tail == NULL) {
        q_head = node;
    } else {
        tail->next = node;
    }
}

static interpolator_target_node_t *
q_dequeue(void)
{
    interpolator_target_node_t * orig_head = q_head;
    if (orig_head != NULL) {
        q_head = orig_head->next;
        orig_head->in_use = false;
        orig_head->next = NULL;
    }

    return orig_head;
}

/*
 * Get a ptr to the current or NULL
 */
static interpolator_target_node_t *
interpolator_get_current_target(void)
{
    return q_head;
}

/*
 * Initialize the linear interpolator
 */
void
interpolator_init(timers_state_t * timers_state)
{
    /* warm up the velocity calculation service */
    int i;
    g_timers_state = timers_state;
    last_position = interpolator_get_current_position();
    current_velocity = 0;
    for (i = 0; i < COUNT_OF(q_nodes); i++) {
        q_nodes[i].in_use = false;
    }
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
	if (q_head != NULL) {
	    uint32_t timedelta;
	    uint16_t delta;
	    switch (state) {
	    case STATE_OUT_OF_ENDZONE:
	        delta = abs(interpolator_get_absolute_target_position() - interpolator_get_current_position());
	        if (delta < CLOSE_ENOUGH_DEGREES) {
	            time_entered_end_zone = g_timers_state->ms_ticks;
	            state = STATE_IN_ENDZONE;
	        }
	        break;
	    case STATE_IN_ENDZONE:
	        timedelta = g_timers_state->ms_ticks - time_entered_end_zone;
//            LOG("%u, %u, %u\r\n", g_timers_state->ms_ticks, time_entered_end_zone, timedelta);
	        if (timedelta > ENDZONE_MS) {
                state = STATE_OUT_OF_ENDZONE;
                q_dequeue();
            }
	        break;
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
    q_append(target_position);
}

/*
 * Get the current interpolator adjusted target position (absolute)
 */
int32_t
interpolator_get_target_position(void)
{
    interpolator_target_node_t * t = interpolator_get_current_target();
    if (t != NULL) {
        int32_t delta = t->position - interpolator_get_current_position();
        if (delta > MAX_DELTA) {
            return interpolator_get_current_position() + MAX_DELTA;
        } else if (delta < -MAX_DELTA) {
            return interpolator_get_current_position() - MAX_DELTA;
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
    interpolator_target_node_t * t = interpolator_get_current_target();
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
    interpolator_target_node_t * t = q_get_tail();
    if (t != NULL) {
        return interpolator_add_target_position(t->position + degrees_delta);
    } else {
        return interpolator_add_target_position(interpolator_get_current_position() + degrees_delta);
    }
}

