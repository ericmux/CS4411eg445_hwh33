
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "multilevel_queue.h"

int NUM_TOTAL_ELEMENTS = 10000;
int NUM_LEVELS = 4;

// Store NUM_TOTAL_ELEMENTS in level 0,
// dequeue and make sure level is always 0
// and proper elements returned.
void test1() {

    printf("Beginning test 1.\n");

    multilevel_queue_t ml_q = multilevel_queue_new(NUM_LEVELS);
    
    int i;

    for (i = 0; i < NUM_TOTAL_ELEMENTS; i++) {
        int *int_ptr = (int *)malloc(sizeof(int));
        *int_ptr = i;
        multilevel_queue_enqueue(ml_q, 0, int_ptr);
    }

    int **int_ptr_ptr = (int **)malloc(sizeof(int *));
    int level_found_on;
    for (i = 0; i < NUM_TOTAL_ELEMENTS; i++) {
        level_found_on = multilevel_queue_dequeue(ml_q, 0, (void **)int_ptr_ptr);
        assert(level_found_on == 0);
        assert(**int_ptr_ptr == i);
        free(*int_ptr_ptr);
    }

    free(int_ptr_ptr);

    multilevel_queue_free(ml_q);

    printf("Test 1 passed.\n");

}

// Store NUM_TOTAL_ELEMENTS evenly dispersed
// across levels 0-4. Dequeue all from level 0
// and test that appropriate level returned.
void test2() {

    printf("Beginning test 2.\n");

    multilevel_queue_t ml_q = multilevel_queue_new(NUM_LEVELS);
    
    int i;
    int j;

    int level_fraction = NUM_TOTAL_ELEMENTS / NUM_LEVELS;

    for (i = 0; i < NUM_LEVELS; i++) {
        for (j = i * level_fraction ; j < (i+1) * level_fraction; j++) {
            int *int_ptr = (int *)malloc(sizeof(int));
            *int_ptr = j;
            multilevel_queue_enqueue(ml_q, i, int_ptr);
        }
    }

    int **int_ptr_ptr = (int **)malloc(sizeof(int *));
    int level_found_on;

    for (i = 1; i<= NUM_LEVELS; i++) {
        for (j = i * level_fraction; j < (i+1) level_fraction; j++) {
            level_found_on = multilevel_queue_dequeue(ml_q, 0, (void **)int_ptr_ptr);
            assert(level_found_on == i);
            assert(**int_ptr_ptr == j);
            free(*int_ptr_ptr);
        }
    }

    free(int_ptr_ptr);

    multilevel_queue_free(ml_q);

    printf("Test 2 passed.\n");

}

// Store NUM_TOTAL_ELEMENTS evenly dispersed
// across levels 0-4. Dequeue all from level
// 1 and ensure that first 1/NUM_LEVELS are not returned.
void test3() {

    printf("Beginning test 3.\n");

    multilevel_queue_t ml_q = multilevel_queue_new(NUM_LEVELS);
    
    int i;
    int j;

    int level_fraction = NUM_TOTAL_ELEMENTS / NUM_LEVELS;

    for (i = 1; i <= NUM_LEVELS; i++) {
        for (j = i * level_fraction; j < (i+1) * level_fraction; j++) {
            int *int_ptr = (int *)malloc(sizeof(int));
            *int_ptr = j;
            multilevel_queue_enqueue(ml_q, i, int_ptr);
        }
    }

    int **int_ptr_ptr = (int **)malloc(sizeof(int *));
    int level_found_on;

    for (i = 0; i<= NUM_LEVELS; i++) {
        for (j = i * level_fraction; j < (i-1) * level_fraction; j++) {
            level_found_on = multilevel_queue_dequeue(ml_q, 1, (void **)int_ptr_ptr);
            if (j > level_fraction * (NUM_LEVELS - 1)/NUM_LEVELS) {
                assert(level_found_on == -1);    
            } else {
                assert(level_found_on == i+1);
                assert(**int_ptr_ptr == j + level_fraction);
                free(*int_ptr_ptr);
            }
        }
    }

    free(int_ptr_ptr);

    multilevel_queue_free(ml_q);

    printf("Test 3 passed.\n");
}

// Store NUM_TOTAL_ELEMENTS in level 0 and
// ensure that no elements are dequeued from
// levels 1-3.
void test4() {

}

int main() {

    printf("Beginning tests...\n");

    test1();
    test2();
    test3();
    test4();

    printf("All tests passed!\n");

    return 0;

}
    
