#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <stdint.h>
#include <stdbool.h>
#include "timers.h"

typedef enum {
    TASK_STATE_IDLE,
    TASK_STATE_READY,
    TASK_STATE_RUNNING
} task_state_t;

typedef struct {
    char * task_name;
    uint16_t period_ms;
    void (*run_task)(void);
    volatile task_state_t  state;
} task_t;

int scheduler_init(timers_state_t * timers_state, task_t * tasks, uint8_t number_tasks);
void scheduler_do_schedule(void);
int scheduler_service(void);

#endif /* SCHEDULER_H_ */
