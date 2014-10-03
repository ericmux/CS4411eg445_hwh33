#include <stdio.h>
#include <stdlib.h>

#include "interrupts.h"
#include "alarm.h"
#include "minithread.h"
#include "queue.h"


//Private time reference for the alarms.
long *current_tick_ptr;
int clock_period = MINITHREAD_CLOCK_PERIOD; 

// nodes in the doubly-linked list
typedef struct node {
    alarm_t alarm;
    node *next_node;
    node *previous_node;
}

// The alarm list stores the alarms. We only need to track
// the head.
// INVARIANT: each alarm is set to go off at some point AFTER
// the alarm before it (essentially, the list is ordered by
// the alarm's scheudled times to go off, where the head is the
// next scheduled alarm)
node alarm_list_head;

typedef struct alarm {
	long 			trigger_tick;
	alarm_handler_t handler;
	void* 			arg;
	int 			executed;
} alarm;
typedef alarm *alarm_t;

/* see alarm.h */
alarm_id
register_alarm(int delay, alarm_handler_t alarm, void *arg)
{
	alarm_t p_alarm;
    node current_node;
    node new_alarm_node;

    alarm_t new_alarm = (alarm_t) malloc(sizeof(alarm));
    new_alarm->trigger_tick = *current_tick_ptr + (delay / clock_period) + 1;
    new_alarm->handler = alarm;
    new_alarm->arg = arg;
    new_alarm->executed = 0;

    // Create a node for new_alarm
    new_alarm_node = (node) malloc(sizeof(node));
    new_alarm_node->alarm = new_alarm;

    //Find the alarm's position in the list
    current_node = alarm_list_head;
    while (current_node != NULL) {
        if (current_node->alarm->trigger_tick >= new_alarm->trigger_tick) {
            break;
        }
    }
    // Now we want to insert a node for new_alarm before
    

    //Remove all the alarms which will trigger earlier.
    while(queue_dequeue(alarm_queue, (void **) &p_alarm) == 0){
    	queue_prepend(buffer_queue, p_alarm);

    	if(new_alarm->trigger_tick <= p_alarm->trigger_tick) break;
    }

    //Add this new alarm to the queue.
    queue_prepend(alarm_queue, new_alarm);

    //Rebuild the rest of the queue.
    while(queue_dequeue(buffer_queue, (void **) &p_alarm) == 0){
    	queue_prepend(alarm_queue, p_alarm);
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

alarm_id pop_alarm(){
	alarm_t best_alarm;
	
	if(queue_dequeue(alarm_queue, (void **) &best_alarm) == 0){
		if(best_alarm->trigger_tick > *current_tick_ptr){
			queue_prepend(alarm_queue, best_alarm);
			best_alarm = NULL;
		}
	}

	return best_alarm;
}

void execute_alarm(alarm_id alarm){
	alarm_t a = alarm;
	a->handler(a->arg);
	a->executed = 1;
}


void initialize_alarm_system(int period, long *tick_pointer){
	clock_period = period;
	current_tick_ptr = tick_pointer;	
    // we don't have any alarms, so our head is null
    alarm_list_head = NULL
}


/*
** vim: ts=4 sw=4 et cindent
*/
