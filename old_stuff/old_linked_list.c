/*
 * An ordered doubly-linked list implementation 
 */

typedef struct node {
	void *item_ptr;  // to hold data in this node. cannot be NULL
	long priority;  // head has lowest priority, each node after has >= priority
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
// On error, NULL is stored in item
int linked_list_pop_head(linked_list_t linked_list, void **item) {
	node *old_head;
	
	if (linked_list == NULL || linked_list->head->item_ptr == NULL) {
		*item = NULL;
		return -1;
	}

	*item = linked_list->head->data;

	// set the new head and free the old head
	old_head = linked_list->head;
	linked_list->head = linked_list->head->next_node;
	free(old_head);  // maybe not do this
	if (linked_list->head != NULL) {
		linked_list->head->previous_node = NULL;
	}
	
	return 0;
}

// Stores the pointer item in a new node. Inserts the node in
// the linked list according to input_priority. Does not allow 
// item to be NULL.
int linked_list_insert(linked_list_t linked_list, void *item, long priority) {
	node *new_node;
	node *current_node;

	if (linked_list == NULL || item == NULL) return -1;
	
	// Set up our new node. For now, we initialize next
	// and previous node to be NULL
	new_node = (node *)malloc(sizeof(node));
	new_node->item_ptr = item;
	new_node->priority = input_priority;
	new_node->previous_node = NULL;
	new_node->next_node = NULL;

	// We crawl the list and look for the location at which we need
	// to insert the new node	
	current_node = linked_list->head;
	while (current_node != NULL) {
		if (new_node->priority <= current_node->priority) {
			// We have found the node we want to insert our new node before
			new_node->next_node = current_node;
			new_node->previous_node = current_node->previous_node;
			current_node->previous_node = new_node;
			break;
		}
		if (current_node->next_node == NULL) {
			// We have reached the end of the list, so our new node needs
			// to go after the current node
			new_node->previous_node = current_node;
			current_node->next_node = new_node;
		}
		current_node = current_node->next_node;
	}

	// We check the new node's pointers. If they are both still unset,
	// then the while loop was skipped, reflecting an empty list. So
	// our new node is the first node, and thus the head
	if (new_node->next_node == NULL && new_node->previous_node == NULL) {
		linked_list->head = new_node;
	}

	return 0;
}


