#include <stdio.h>
#include <stdlib.h>

#include "interrupts.h"
#include "alarm.h"
#include "minithread.h"
#include "queue.h"


//Private time reference for the alarms.
long *current_tick_ptr;
int clock_period = MINITHREAD_CLOCK_PERIOD; 

//Priority queue for alarms.
queue_t alarm_queue;

//Second buffer queue to store thread which are registered to wake up earlier.
queue_t buffer_queue;


typedef struct alarm_t {
	long 			trigger_tick;
	alarm_handler_t handler;
	void* 			arg;
	int 			executed;
} *alarm_t;

/* see alarm.h */
alarm_id
register_alarm(int delay, alarm_handler_t alarm, void *arg)
{
	alarm_t *alarm_ptr = NULL;

    alarm_t new_alarm = (alarm_t) malloc(sizeof(struct alarm_t));
    new_alarm->trigger_tick = *current_tick_ptr + (delay / clock_period);
    new_alarm->handler = alarm;
    new_alarm->arg = arg;
    new_alarm->executed = 0;

    //Find rightful position in the priority queue.

    //Remove all the alarms which will trigger earlier.
    while(queue_dequeue(alarm_queue, (void **) alarm_ptr) == 0){
    	alarm_t v = *alarm_ptr;
    	queue_prepend(buffer_queue, v);

    	if(new_alarm->trigger_tick <= v->trigger_tick) break;
    }

    //Add this new alarm to the queue.
    queue_prepend(alarm_queue, new_alarm);

    //Rebuild the rest of the queue.
    while(queue_dequeue(buffer_queue, (void **) alarm_ptr) == 0){
    	alarm_t v = *alarm_ptr;
    	queue_prepend(alarm_queue, v);
    }


    return (alarm_id) new_alarm;
}

/* see alarm.h */
int
deregister_alarm(alarm_id alarm)
{
	alarm_t a = (alarm_t) alarm;

	if(a == NULL) return 0;

    if(a->executed){
    	free(a);
    	return 1;
    }

    //TO DO: deal with alarm removal from alarm_queue.


    return 0;
}

alarm_id peek_alarm(){
	alarm_t best_alarm;
	queue_dequeue(alarm_queue, (void **) &best_alarm);
	return best_alarm;
}


void initialize_alarm_system(int period, long *tick_pointer){
	clock_period = period;
	current_tick_ptr = tick_pointer;
	alarm_queue = queue_new();
	buffer_queue = queue_new();
}


/*
** vim: ts=4 sw=4 et cindent
*/
