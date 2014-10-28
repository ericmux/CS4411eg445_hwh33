
/* The original test file was deleted...
 * I don't feel like re-typing it all, but here's
 * what was tested:
 *     - Put 10,000 entries into the table and
 *       checked that all were mapped to by the
 *       appropriate keys.
 *     - Put 10,000 entries into the table, then
 *       deleted a specific 5,000 of them and
 *       checked that the appropriate entries
 *       were deleted and that the appropriate
 *       ones remained.
 *     - Put 10,000 entries into the table with
 *       the same key. Called hashtable_remove
 *       once and checked that the hashtable
 *       removed all entries.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "hashtable.h"

int NUM_ENTRIES;

// Puts NUM_ENTRIES into the table, half with
// data pointer c1 and half with c2, then
// checks that the keys map to the appropriate
// data pointers.
int test4() {
    int i;
    char *c1 = (char *)malloc(sizeof(char));
    char *c2 = (char *)malloc(sizeof(char));
    char **data_retrieved = (char **)malloc(sizeof(char *));

    *c1 = 'H';
    *c2 = '3';

    printf("Beginning test 4.\n");

    hashtable_t table = hashtable_create(); 

    for (i = 0; i < (NUM_ENTRIES/2); i++) {table = hashtable_put(table, i, (void *)c1);}

    for (i = (NUM_ENTRIES/2); i < NUM_ENTRIES; i++) {table = hashtable_put(table, i, (void *)c2);}

    for (i = 0; i < (NUM_ENTRIES/2); i++) {
        hashtable_get(table, i, (void **)data_retrieved);
        assert(**data_retrieved == 'H');
    }

    for (i = 0; i < (NUM_ENTRIES/2); i++) {
        hashtable_get(table, i, (void **)data_retrieved);
        assert(**data_retrieved == '3');
    }

    free(c1);
    free(c2);
    free(data_retrieved);
    hashtable_free(table);

    printf("Test 4 passed.\n");
}

int main() {
    
    printf("Beginning tests...\n");

    test4();

    printf("All tests passed!\n");

    return 0;

}
