#ifndef __ALARM_H__
#define __ALARM_H__ 1

/*
 * This is the alarm interface. You should implement the functions for these
 * prototypes, though you may have to modify some other files to do so.
 */


/* An alarm_handler_t is a function that will run within the interrupt handler.
 * It must not block, and it must not perform I/O or any other long-running
 * computations.
 */
typedef void (*alarm_handler_t)(void*);
typedef void *alarm_id;

/* register an alarm to go off in "delay" milliseconds.  Returns a handle to
 * the alarm.
 */
alarm_id register_alarm(int delay, alarm_handler_t func, void *arg);

/* unregister an alarm.  Returns 0 if the alarm had not been executed, 1
 * otherwise.
 */
int deregister_alarm(alarm_id id);

#endif
