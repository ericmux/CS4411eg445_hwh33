/*
 * An ordered doubly-linked list implementation
 */

#ifndef __LINKED_LIST_H__
#define __LINKED_LIST_H__

typedef struct linked_list *linked_list_t;

extern linked_list_t linked_list_new();

// Pops the first node's item pointer and stores it in the 
// given void pointer-pointer
// returns 0 on success and -1 on failure
extern int linked_list_pop_head(linked_list_t, void**);

// Inserts a node with an item pointer equal to the given
// void pointer. Takes in a priority of type long. Lowest
// priorities are at the head of the list (and popped off
// first).
extern int linked_list_insert(linked_list_t, void*, long)
