/*
* A simple hashtable which stores data pointers
* by integer keys.
*
*/

#include <stdlib.h>
#include <stdio.h> //TODO: for debugging, remove later

#include "hashtable.h"
#include "linked_list.h"

// LOAD_FACTOR defines the largest value we will allow
// (number of elements) / (number of buckets) to take
// before we double the size of the table.
#define LOAD_FACTOR 0.75

// The number of buckets the hashtable is initialized
// to have. The trade-off is that a large number of
// initial buckets will mean less initial expanding of
// the table but longer initialization.  
#define DEFAULT_INITIAL_NUM_BUCKETS 10

// Hash function. This was found at:
// http://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
unsigned int hash(unsigned int x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x);
    return x;
}

typedef struct hashtable {
    int num_buckets;
    int num_elements;
    // buckets is an array of linked_list_t's
    linked_list_t *buckets;
}hashtable;

// An internal hashtable creation method. We have this so that we
// can create new hashtables with variable size to copy data
// over to when we are doubling our hashtable size.
hashtable_t hashtable_create_internal(int num_buckets) {
    int i;
    linked_list_t *new_buckets;
    hashtable_t new_hashtable;

    new_hashtable = (hashtable_t)malloc(sizeof(hashtable));

    // Initialize an empty hashtable with num_buckets
    // empty buckets.
    new_hashtable->num_buckets = num_buckets;
    new_buckets = (linked_list_t *)malloc(num_buckets * sizeof(linked_list_t));
    for (i = 0; i < num_buckets; i++) {
        new_buckets[i] = linked_list_create();
    }
    new_hashtable->buckets = new_buckets;
    new_hashtable->num_elements = 0;

    return new_hashtable;
}

// This way we ensure that all hashtables are initialized with
// INITIALIZATION_NUM_BUCKETS.
hashtable_t hashtable_create() {
    return hashtable_create_internal(DEFAULT_INITIAL_NUM_BUCKETS);
}

// Allows the user to specify how many buckets the hashtable is
// initialized with.
hashtable_t hashtable_create_specify_buckets(int initial_num_buckets) {
    return hashtable_create_internal(initial_num_buckets);
}

// To be called by hashtable_put when the load factor is exceeded.
hashtable_t double_table_size(hashtable_t old_table) {
    int i;
    int key_to_copy;
    void **data_ptr_to_copy = (void **)malloc(sizeof(void *));
    linked_list_t current_bucket;

    // Create a new hashtable of double the size to copy data over into.
    hashtable_t new_table = hashtable_create_internal(2 * old_table->num_buckets);
     
    // Copy all of the old hashtable data over.
    for (i = 0; i < old_table->num_buckets; i++) {
        current_bucket = old_table->buckets[i];
        while (!linked_list_empty(current_bucket)) {
            key_to_copy = linked_list_pop(current_bucket, data_ptr_to_copy);
            hashtable_put(new_table, key_to_copy, *data_ptr_to_copy);
        }
    }

    // Free the memory used by the old hashtable.
    hashtable_free(old_table);

    return new_table;
}

// The programmer must take the returned table as their new table
// as the old one may be overwritten.
hashtable_t hashtable_put(hashtable_t table, int key, void *data) {
    unsigned int hash_value;
    int bucket_number;

    // If this put will cause our load factor to be exceeded, we
    // need to double the size of our table
    float future_load = ((float) table->num_elements + 1) / (float) table->num_buckets;
    if (future_load > LOAD_FACTOR) {
        table = double_table_size(table);
    }

    // We need to determine what bucket this key/value pair will
    // be placed in. We use hash(key) mod N to do so. 
    hash_value = hash((unsigned int) key);
    bucket_number = hash_value % table->num_buckets;

    // Now we append the key to the end of the list in
    // the appropriate bucket and increment the number
    // of elements in our table.
    linked_list_append(table->buckets[bucket_number], key, data);
    table->num_elements++;

    return table;
}

int hashtable_get(hashtable_t table, int key, void **data_ptr) {
    unsigned int hash_value;
    int bucket_number;

    // Find the bucket number the key should be in.
    hash_value = hash((unsigned int) key);
    bucket_number = hash_value % table->num_buckets;

    // Now find the first element mapped to by key in the appropriate bucket.
    return linked_list_get(table->buckets[bucket_number], key, data_ptr);
}

// Removes ALL instances of key from table.
void hashtable_remove(hashtable_t table, int key) {
    unsigned int hash_value;
    int bucket_number;

    // Find the bucket number the key should be in.
    hash_value = hash((unsigned int) key);
    bucket_number = hash_value % table->num_buckets;

    // Now remove all instances of that key from the
    // bucket.
    linked_list_remove(table->buckets[bucket_number], key);
}

int hashtable_key_exists(hashtable_t table, int key) {
    unsigned int hash_value;
    int bucket_number;

    // Find the bucket number the key should be in.
    hash_value = hash((unsigned int) key);
    bucket_number = hash_value % table->num_buckets;

    // Now check for the key in the bucket.
    return linked_list_key_exists(table->buckets[bucket_number], key);
}

void hashtable_free(hashtable_t table) {
    int i;
    linked_list_t current_bucket;

    for (i = 0; i < table->num_buckets; i++) {
        current_bucket = table->buckets[i];
        linked_list_free(current_bucket);
    }

    free(table->buckets);
    free(table);
}
   
