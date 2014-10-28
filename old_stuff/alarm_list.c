/*
 * An ordered doubly-linked list implementation 
 */

#include "alarms.h"

typedef struct node {
	void *item;
	long ordering_value; 
	node *next_node;
	node *previous_node;
}

typedef struct linked_list {
	node *head;
}

typedef linked_list *linked_list_t;

linked_list_t linked_list_new() {
	linked_list_t new_linked_list = (linked_list *)malloc(sizeof(linked_list));
	linked_list->head = NULL;
}

// Returns 0 on success and -1 on error.
// Stores the pointer to the head's data in item.
int linked_list_pop_head(linked_list_t linked_list, void **item) {
	if (linked_list == NULL || linked_list->head->data == NULL) return -1;

	*item = linked_list->head->data;
	return 0;
}

// Stores the pointer item in a new node and 
int linked_list_insert(linked_list_t linked_list, void *item




