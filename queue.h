/*
 * Generic queue manipulation functions
 */
#ifndef __QUEUE_H__
#define __QUEUE_H__

/*
 * queue_t is a pointer to an internally maintained data structure.
 * Clients of this package do not need to know how queues are
 * represented.  They see and manipulate only queue_t's.
 */
typedef struct queue* queue_t;

/*
 * Return an empty queue.  Returns NULL on error.
 */
extern queue_t queue_new();

/*
 * Prepend a void* to a queue (both specifed as parameters).
 * Returns 0 (success) or -1 (failure).
 */
extern int queue_prepend(queue_t, void*);

/*
 * Appends a void* to a queue (both specifed as parameters).  Return
 * 0 (success) or -1 (failure).
 */
extern int queue_append(queue_t, void*);

/*
 * Dequeue and return the first void* from the queue.
 * Return 0 (success) and first item if queue is nonempty, or -1 (failure) and
 * NULL if queue is empty.
 */
extern int queue_dequeue(queue_t, void**);

/*
 * queue_iterate(q, f, t) calls f(x,t) for each x in q.
 * q and f should be non-null.
 *
 * returns 0 (success) or -1 (failure)
 */
typedef void (*func_t)(void*, void*);
extern int queue_iterate(queue_t, func_t, void*);

/*
 * Free the queue and return 0 (success) or -1 (failure).
 */
extern int queue_free (queue_t);

/*
 * Return the number of items in the queue, or -1 if an error occured
 */
extern int queue_length(queue_t queue);

/*
 * Delete the first instance of the specified item from the given queue.
 * Returns 0 if an element was deleted, or -1 otherwise.
 */
extern int queue_delete(queue_t queue, void* item);

#endif /*__QUEUE_H__*/
