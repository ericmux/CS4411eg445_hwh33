/*
 * Multilevel queue manipulation functions  
 */
#include "multilevel_queue.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct multilevel_queue {
	// Each element of queue_array is a queue. The ith-element
	// is the queue at level i. When the array is full,  
	queue_t *queue_array;
	int number_of_levels;
} multilevel_queue;

/*
 * Returns an empty multilevel queue with number_of_levels levels. On error should return NULL.
 * A value < 1 on number_of_levels is treated as an error (and thus returns NULL).
 */
multilevel_queue_t multilevel_queue_new(int number_of_levels)
{
	multilevel_queue_t new_multilevel_queue;
	queue_t *new_queue_array;
        int i;

	if (number_of_levels < 1) return NULL;
	
	new_multilevel_queue = (multilevel_queue_t) malloc(sizeof(multilevel_queue));
	new_queue_array = (queue_t *)malloc(sizeof(queue_t) * number_of_levels);
        for (i = 0; i < number_of_levels; i++) {
            new_queue_array[i] = queue_new();
        }
	new_multilevel_queue->queue_array = new_queue_array;
	new_multilevel_queue->number_of_levels = number_of_levels;
	return new_multilevel_queue;
}

/*
 * Appends an void* to the multilevel queue at the specified level. Return 0 (success) or -1 (failure).
 */
int multilevel_queue_enqueue(multilevel_queue_t queue, int level, void* item)
{
	if (queue == NULL || level < 0 || level > queue->number_of_levels - 1) return -1;

	return queue_append(queue->queue_array[level], item);
}

/*
 * Dequeue and return the first void* from the multilevel queue starting at the specified level. 
 * Levels wrap around so as long as there is something in the multilevel queue an item should be returned.
 * Return the level that the item was located on and that item if the multilevel queue is nonempty,
 * or -1 (failure) and NULL if queue is empty.
 */
int multilevel_queue_dequeue(multilevel_queue_t queue, int level, void** item)
{
	int found_item;
	int current_level;

	if (queue == NULL || level < 0 || level > queue->number_of_levels - 1) return -1;

	found_item = 0;
	current_level = 0;
	*item = NULL;
	while (!found_item && current_level < queue->number_of_levels) {
		// queue_dequeue returns -1 on error and 0 on success. So found_item
		// will be 0 (false) if the item was not found and 1 (true) if it was.
		found_item = 1 + queue_dequeue(queue->queue_array[current_level], item);
		current_level++;
	}

	// If anything was found, then item was set in the loop. Otherwise, it's still NULL.
	// found_item is 1 (true) if we found an item and 0 (false) if we did not. So
	// found_item - 1 will be 0 on success and -1 on error.
	return found_item - 1;
}

/* 
 * Free the queue and return 0 (success) or -1 (failure). Do not free the queue nodes; this is
 * the responsibility of the programmer.
 */
int multilevel_queue_free(multilevel_queue_t queue)
{
	int i;
	int error_occurred;
	int exit_code;

	if (queue == NULL) return -1;

	error_occurred = 0;
	for (i = 0; i < queue->number_of_levels; i++) {
		exit_code = queue_free(queue->queue_array[i]);
		if (exit_code == -1) error_occurred = 1;
	}
	free(queue);

	// error_occurred is a boolean, so 1 indicates an error and 0 indicates a
	// clean execution. error_occurred * -1 will thus return -1 on error and 0
	// on success
	return error_occurred * -1;		
}



