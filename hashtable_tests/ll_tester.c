
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "linked_list.h"

int test1() {
    int i;

    linked_list_t list = linked_list_create();
    char *c = (char *)malloc(sizeof(char));
    *c = 'H';

    for (i = 0; i < 100; i++) {linked_list_append(list, i, c);}

    char **data = (char **)malloc(sizeof(char*));
    while (!linked_list_empty(list)) {
        linked_list_pop(list, (void **)data);
        assert(**data == 'H');
    }

    linked_list_free(list);
}

// Will this cause a memory leak on the tail?
int test2() {

    char *c = (char *)malloc(sizeof(char));
    *c = '3';

    linked_list_t list = linked_list_create();
    linked_list_append(list, 3, c);
    linked_list_remove(list, 3);

    linked_list_free(list);
}

int main() {
    test1();

    test2();

    printf("All tests passed!\n");

    return 0;
}
