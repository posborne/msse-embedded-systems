#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "scheduler.h"

static task_t * g_tasks = NULL;
static uint8_t g_number_tasks = 0;
static timers_state_t * g_timers_state;

/*
 * Initialize the scheduler
 */
int
scheduler_init(timers_state_t * timers_state, task_t * tasks, uint8_t number_tasks)
{
    int i;
    g_timers_state = timers_state;
    g_tasks = tasks;
    g_number_tasks = number_tasks;
    for (i = 0; i < number_tasks; i++) {
        tasks[i].state = TASK_STATE_IDLE;
    }
    return 0;
}

/*
 * Called from ISR.  Release appropriate tasks to be called from main thread
 */
void
scheduler_do_schedule(void)
{
    int i;
    task_t * task;
    for (i = 0; i < g_number_tasks; i++) {
        task = &g_tasks[i];
        if (g_timers_state->ms_ticks % task->period_ms == 0) {
            task->state = true;
        }
    }
}

/*
 * It is expected that will be called frequently from the main thread
 *
 * This will execute any tasks that are released in the order they were
 * provided in the initial task list.  There is no preemption.
 */
int
scheduler_service(void)
{
    int i;
    task_t * task;
    for (i = 0; i < g_number_tasks; i++) {
        task = &g_tasks[i];
        if (task->state == TASK_STATE_READY) {
            task->state = TASK_STATE_RUNNING;
            task->run_task();
            task->state = TASK_STATE_IDLE;
        }
    }
    return 0;
}
