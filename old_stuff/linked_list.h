/*
 * A simple linked-list of integers intended for
 * use in a special hashtable which only stores keys.
 * Only necessary methods are defined, so this isn't
 * really a full implementation.
 *
 */

#ifndef __LINKED_LIST_H__
#define __LINKED_LIST_H__

// A pointer to a linked_list.
typedef struct linked_list* linked_list_t;

// Returns a new linked_list_t representing an empty list.
extern linked_list_t linked_list_create();

// Returns 1 if the list is empty and 0 otherwise.
extern int linked_list_empty(linked_list_t);

// linked_list_append(list, n) appends n to the end of
// the list.
extern void linked_list_append(linked_list_t, int);

// linked_list_remove_key(list, n) removes n from the list.
extern void linked_list_remove(linked_list_t, int);

// linked_list_pop(list) removes the head of the list and
// returns its value. Removing the tail might be more logical,
// but removing the head is more efficient and works fine
// for our purposes.
// NOTE: assumes that the list is non-empty. It is the
// programmer's responsibility to check first using
// linked_list_empty.
extern int linked_list_pop(linked_list_t);

// linked_list_key_exists(list, n) returns 1 if n is found 
// in the list and 0 otherwise.
extern int linked_list_number_exists(linked_list_t, int);

// Frees the memory allocated to the given list.
extern void linked_list_free(linked_list_t);

#endif /*__LINKED_LIST_H__*/
