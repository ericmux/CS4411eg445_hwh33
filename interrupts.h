/*
 * Provides a clean, virtualized interface to interrupts.
 *
 * We provide a virtual processor, which acts much like a
 * modern, real processor - it responds to interrupts, and
 * takes them on the current stack.
 *
 * Initially, when the system starts up, interrupts are disabled.
 * Calling minithreads_clock_init will start the clock device and
 * enable interrupts.
 *
 * Interrupts are disabled when running code that is not part of the
 * minithreads package (e.g. printf or gettimeofday), or if they are explicitly
 * disabled (see set_interrupt_level below).  Any interrupts that occur while
 * interrupts are disabled will be dropped.  Thus if you want to reliably
 * receive interrupts, you must avoid spending a large portion of time with
 * interrupts disabled.
 *
 * YOU SHOULD NOT [NEED TO] MODIFY THIS FILE.
 */

#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__ 1

#include "defs.h"

/* set_interrupt_level(interrupt_level_t level)
 *      Set the interrupt level to newlevel, return the old interrupt level
 *
 * You should generally make changes to the interrupt level in a set/restore
 * pair. Be careful about restoring the interrupt level. Your
 * protected code may be have been called with interrupts already
 * disabled, in which case blindly reenabling interrupts will cause
 * synchronization problems. Rather than downgrading the interrupts
 * to a particular level without reference to the old value, you should
 * generally use a set-and-restore scheme, as follows:

     interrupt_level_t l;
     ...
     l = set_interrupt_level(DISABLED);
     ... [protected code]
     set_interrupt_level(l);

 * this way, you are protected against nested interrupt downgrades (i.e.
 * function A disables interrupts and calls B, which also disables them. If
 * B enables them, instead of setting the interrupt_level to its old value,
 * interrupts will be enabled when B terminates, when A expected them to be
 * disabled.
 *
 * the exception to this is when you're disabling interrupts before a call
 * to minithread_switch: the minithread switch code resets the interrupt
 * level to ENABLED itself.
 *
 * Interrupts that occur while interrupts are disabled are dropped, so you
 * should minimize the amount of time interrupts are disabled in order to
 * reduce the number of dropped interrupts.
 */

typedef int interrupt_level_t;
extern interrupt_level_t interrupt_level;

#define DISABLED 0
#define ENABLED 1

extern interrupt_level_t set_interrupt_level(interrupt_level_t newlevel);


/*
 * minithread_clock_init(h,period)
 *     installs a clock interrupt service routine h.  h will be called every
 *     [period] nanoseconds.  interrupts are disabled after
 *     minithread_clock_init finishes.  After you enable interrupts then your
 *     handler will be called automatically on every clock tick.
 */
#define NANOSECOND  1
#define MICROSECOND (1000*NANOSECOND)
#define MILLISECOND (1000*MICROSECOND)
#define SECOND      (1000*MILLISECOND)

typedef void(*interrupt_handler_t)(void*);
extern void minithread_clock_init(int period, interrupt_handler_t h);

#endif /* __INTERRUPTS_H__ */

