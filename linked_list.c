/*
 * A simple linked-list of intended for use in a 
 * hashtable. Only necessary methods are defined, 
 * so this isn't really a full implementation.
 *
 */

#include <stdlib.h>
#include <stdio.h> // for debugging, remove later

#include "linked_list.h"

typedef struct node {
    int key;
    void *data;
    struct node *next;
} node;

typedef struct linked_list {
    node *head;
    node *tail;
} linked_list;

linked_list_t linked_list_create() {
    linked_list_t new_linked_list = (linked_list_t) malloc(sizeof(linked_list));

    new_linked_list->head = NULL;
    new_linked_list->tail = NULL;

    return new_linked_list;
}

int linked_list_empty(linked_list_t list) {
    return (list->head == NULL);
}

void linked_list_append(linked_list_t list, int key, void *data) {
    node *new_node = (node *)malloc(sizeof(node));

    new_node->key = key;
    new_node->data = data;
    new_node->next = NULL;

    // Check if this is the first element in the list.
    if (list->head == NULL) {
        list->head = new_node;
        list->tail = new_node;
        return;
    }

    list->tail->next = new_node;
    list->tail = new_node;
}

// Removes ALL instances of key_to_remove from the list.
// Does nothing if the number is not found.
// The programmer is reponsible for freeing the data stored.
void linked_list_remove(linked_list_t list, int key_to_remove) {
    node *previous_node = NULL;
    node *current_node = list->head;
    node *node_to_remove = NULL;

    while (current_node != NULL) {
        if (current_node->key == key_to_remove) {
            node_to_remove = current_node;
            if (node_to_remove == list->head) {
                list->head = node_to_remove->next;
            } else {
                previous_node->next = node_to_remove->next;
            }
            current_node = node_to_remove->next;
            free(node_to_remove);
        } else {
            previous_node = current_node;
            current_node = current_node->next;
        }
    }
}

// Assumes a non-empty list (programmer must test first using
// linked_list_empty). Returns the popped key and stores the 
// popped node's data in data_ptr. 
int linked_list_pop(linked_list_t list, void **data_ptr) {
    int key_to_return;
    node *node_to_free;

    key_to_return = list->head->key;
    *data_ptr = list->head->data;
    node_to_free = list->head;
    list->head = node_to_free->next;
    free(node_to_free);

    return key_to_return;
}

int linked_list_get(linked_list_t list, int key_to_find, void **data_ptr) {
    node *current_node = list->head;

    while (current_node != NULL) {
        if (current_node->key == key_to_find) {
            *data_ptr = current_node->data;
            return 0;
        }
    }

    // If this point is reached, the key was not found.
    *data_ptr = NULL;
    return -1;
}

// Returns 1 if key_to_find is in the list and 0 otherwise.
int linked_list_key_exists(linked_list_t list, int key_to_find) {
    node *current_node = list->head;

    while (current_node != NULL) {
        if (current_node->key == key_to_find) return 1;

        current_node = current_node->next;
    }

    // If this point is reached, the key was not found.
    return 0;
}

// The programmer is reponsible for freeing the data stored.
void linked_list_free(linked_list_t list) {
    node *current_node = list->head;
    node *next_node = NULL;

    while (current_node != NULL) {
        next_node = current_node->next;
        free(current_node);
        current_node = next_node;
    }

    free(list);
}
