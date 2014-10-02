#include <stdio.h>

#include "interrupts.h"
#include "alarm.h"
#include "minithread.h"
#include "queue.h"

/* see alarm.h */
alarm_id
register_alarm(int delay, alarm_handler_t alarm, void *arg)
{
    return NULL;
}

/* see alarm.h */
int
deregister_alarm(alarm_id alarm)
{
    return 1;
}

/*
** vim: ts=4 sw=4 et cindent
*/
