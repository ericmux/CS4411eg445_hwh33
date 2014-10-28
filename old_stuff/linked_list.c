/*
 * A simple linked-list of integers intended for
 * use in a special hashtable which only stores keys.
 * Only necessary methods are defined, so this isn't
 * really a full implementation.
 *
 */

#include <stdlib.h>
#include <stdio.h> // for debugging, remove later

#include "linked_list.h"

typedef struct node {
    int number;
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

void linked_list_append(linked_list_t list, int new_number) {
    node *new_node = (node *)malloc(sizeof(node));

    new_node->number = new_number;
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

// Removes ALL instances of number_to_remove from the list.
// Does nothing if the number is not found.
void linked_list_remove(linked_list_t list, int number_to_remove) {
    node *previous_node = NULL;
    node *current_node = list->head;
    node *node_to_remove = NULL;

    while (current_node != NULL) {
        if (current_node->number == number_to_remove) {
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
// linked_list_empty).
int linked_list_pop(linked_list_t list) {
    int to_return = list->head->number;
    node *node_to_free = list->head;
    list->head = node_to_free->next;
    free(node_to_free);

    return to_return;
}

// Returns 1 if number_to_find is in the list and 0 otherwise.
int linked_list_number_exists(linked_list_t list, int number_to_find) {
    node *current_node = list->head;

    while (current_node != NULL) {
        if (current_node->number == number_to_find) return 1;

        current_node = current_node->next;
    }

    // If this point is reached, the number was not found.
    return 0;
}

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
