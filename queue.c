/*
 * Generic queue implementation.
 *
 */
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

// The queue is backed by a doubly-linked list. Appending adds to the tail
// and prepending adds to the head. Dequeuing removes from the head.	 

// These nodes make up the doubly-linked list. We define the head of the 
// list to be on the left and the tail on the right. These directional 
// definitions have no real meaning, but if NodeA is "left" of NodeB, 
// then we know that NodeA is closer to the head.
typedef struct node {
	void *data_ptr; 		// pointer to the data this node stores
	struct node *left_node;		// the node "left" of this node
	struct node *right_node;	// the node "right" of this node
	int is_head;			// 1 if node is head, 0 otherwise
	int is_tail;			// 1 if node is tail, 0 otherwise
} node;

typedef struct queue {
	node *head;
	node *tail;
	int length;
} queue;

//typedef struct queue* queue_t;

/*
 * Return an empty queue.
 */
queue_t queue_new() {
	queue_t new_queue = (queue *)malloc(sizeof(queue));
	// initialize the first (empty) node:
	node *first_node = (node *)malloc(sizeof(node));
	first_node->data_ptr = NULL;
	first_node->left_node = NULL;
	first_node->right_node = NULL;
	first_node->is_head = 1;
	first_node->is_tail = 1;
	// this node becomes both the head and tail of our empty queue
	new_queue->head = first_node;
	new_queue->tail = first_node;
	// though there is one node, it is empty; it's a dummy node and 
	// should not count towards the length
	new_queue->length = 0;

	return new_queue;
}

/*
 * Prepend a void* to a queue (both specifed as parameters).  Return
 * 0 (success) or -1 (failure).
 */
int
queue_prepend(queue_t queue, void* item) {

	// create a new node to hold the item
	node *new_node;

	// If this is the first node, all we need to do is set the "dummy"
	// node's data pointer:
	if ((queue->length) == 0) {
		(queue->head)->data_ptr = item;
		(queue->length)++;
		return 0;
	} else if ((queue->length) < 0) {
		return -1;
	}

	new_node = (node *)malloc(sizeof(node));
	new_node->data_ptr = item;
	// this node is going to be the head, so it will be left of
	// the old head
	new_node->left_node = NULL;
	new_node->right_node = queue->head;
	new_node->is_head = 1;
	new_node->is_tail = 0;

	// Our new node is now the head, so redirect the old head's pointer
	// to point to our new node. Also, change the old head's status as
	// head. Finally, set the head to the new node.
	(queue->head)->left_node = new_node;
	(queue->head)->is_head = 0;
	queue->head = new_node; 

	(queue->length)++;

	return 0;
}

/*
 * Append a void* to a queue (both specifed as parameters). Return
 * 0 (success) or -1 (failure).
 */
int
queue_append(queue_t queue, void* item) {

	// create a new node to hold the item
	node *new_node;
	new_node = (node *) malloc(sizeof(node));

	// If this is the first node, all we need to do is set the "dummy"
	// node's data pointer:
	if ((queue->length) == 0) {
                (queue->head)->data_ptr = item;
		(queue->length)++;
                return 0;
        } else if (queue->length < 0) {
		return -1;
	}

    new_node->data_ptr = item;
	// this node is going to be the tail, so it will be right of
	// the old tail
	new_node->left_node = queue->tail;
	new_node->right_node = NULL;
	new_node->is_head = 0;
	new_node->is_tail = 1;

	// Our new node is now the tail, so redirect the old tail's pointer 
	// to point to our new node. Also, change the old tail's status as
	// tail. Finally, set the tail to the new node.
	(queue->tail)->right_node = new_node;
        (queue->tail)->is_tail = 0;
        queue->tail = new_node;	

	(queue->length)++;

    	return 0;
}

/*
 * Dequeue and return the first void* from the queue or NULL if queue
 * is empty.  Return 0 (success) or -1 (failure).
 */
int
queue_dequeue(queue_t queue, void** item) {

	node *new_head;
	// first check if the queue is empty
	if ((queue->length) <= 0) {
		*item = NULL;
		return -1;
	}

	// Now we know that there is at least one node, so we take from 
	// the head:
	*item = queue->head->data_ptr;
	queue->length--;

	// If the length is now 0, then we need to set up a new dummy node:
	if (queue->length == 0) {
		node *new_node;
		free(queue->head);
		new_node = (node *)malloc(sizeof(node));
		new_node->data_ptr = NULL;
		new_node->left_node = NULL;
		new_node->right_node = NULL;
		new_node->is_head = 1;
		new_node->is_tail = 1;
		queue->head = new_node;
		queue->tail = new_node;
		return 0;
	}

	// If the length is now 1, we just need to set the old tail to also be
	// the new head (but it remains the tail!)
	if (queue->length == 1) {
		free(queue->head);
		queue->tail->left_node = NULL;
		queue->tail->is_head = 1;
		queue->head = queue->tail;
		return 0;
	}

	// Otherwise, we just set the node right of the head to be the new head
	new_head = queue->head->right_node;
	new_head->left_node = NULL;
	new_head->is_head = 0;
	free(queue->head);
	queue->head = new_head;
	return 0;
}

/*
 * Iterate the function parameter over each element in the queue.  The
 * additional void* argument is passed to the function as its first
 * argument and the queue element is the second.  Return 0 (success)
 * or -1 (failure). ** DOCUMENTATION CONFLICT, FOLLOWING HEADER FILE
 */
int
queue_iterate(queue_t queue, func_t f, void* item) {
    	// we cannot iterate over an empty queue
    	if (queue->length <= 0) {
		return -1;
	}

	// apply f to the head, then iterate until the tail is reached
	node *current_node = queue->head;
	f(current_node, item);		// TODO: see if this is correct
	while (!current_node->is_tail) {
		current_node = current_node->right_node;
		f(current_node, item);	// TODO: this too
	}

	return 0;
}

/*
 * Free the queue and return 0 (success) or -1 (failure).
 */
int
queue_free (queue_t queue) {
	// TODO: can I use queue_iterate here somehow?
	// TODO: check implementation

	node *current_node;

	if (queue->length < 0) {
		return -1;
	}

	if (queue->length == 0) {
		free(queue->head);
		free(queue);
		return 0;
	}

	current_node = queue->head;
	node *next_node;
	while (!current_node->is_tail) {
		next_node = current_node->right_node;
		free(current_node->data_ptr);
		free(current_node);
		current_node = next_node;
	}
	free(current_node->data_ptr);
	free(current_node);
	free(queue);

    	return 0;
}

/*
 * Return the number of items in the queue.
 */
int
queue_length(queue_t queue) {
    	return queue->length;
}


/*
 * Delete the specified item from the given queue.
 * Return -1 on error. ** HEADER SAYS JUST FIRST INSTANCE
 */
int
queue_delete(queue_t the_queue, void* item) {

	int found_item;
	found_item = 0;
	// can't delete from an empty queue
	if (the_queue->length <=0 ){
		return -1;
	} 

	node *current_node = the_queue->head;
	// first check the head:
	if (current_node->data_ptr == item) {
		found_item = 1;
		// the current node's right node is the new head
		if (the_queue->length == 1) {
			// If this is the case, then we are deleting the
			// last node and need to create a new "dummy" node
			node *new_node = (node *)malloc(sizeof(node));
			new_node->data_ptr = NULL;
			new_node->is_head = 1;
			new_node->is_tail = 1;
			new_node->left_node = NULL;
			new_node->right_node = NULL;
			the_queue->head = new_node;
			the_queue->tail = new_node;
		}
		else {
			the_queue->head = current_node->right_node;
			the_queue->head->is_head = 1;
			the_queue->head->left_node = NULL;
		}
		free(current_node);
		the_queue->length--;
		return 0;
	}
	while(!found_item && !current_node->is_tail) {
		current_node = current_node->right_node;				
		if (current_node->data_ptr == item) {
			found_item = 1;
			if (current_node->is_tail) {
				the_queue->tail = current_node->left_node;
				the_queue->tail->is_tail = 1;
				the_queue->tail->right_node = NULL;
			} else {	
				// connect the current node's neighbors together
				(current_node->left_node)->right_node = current_node->right_node;
				(current_node->right_node)->left_node = current_node->left_node;
			}
			free(current_node);
			the_queue->length--;
			return 0;
		}		
	}
    	
	// if this point is reached, then item was not found
	return -1;
}
