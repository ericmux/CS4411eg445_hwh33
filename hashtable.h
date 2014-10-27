/*
* A simple "boolean" hashtable which only stores keys
* This hashtable is useful if we only care that a key 
* exists in the table.
*
*/

#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

// A pointer to a hashtable.
typedef struct hashtable* hashtable_t;

// Returns a new hashtable_t representing an empty table.
extern hashtable_t hashtable_create();

// hashtable_put(table, key, data_ptr) stores a data pointer 
// in the table with key k. 
// The programmer MUST take the returned hashtable as the 
// updated version of their old one as the old one may be 
// overwritten during a size change. Example usage:
//     table = hashtable_put(table, k);
extern hashtable_t hashtable_put(hashtable_t, int, void*);

// hashtable_get(table, key, data_ptr) stores the data
// mapped to by key in data_ptr. Returns 0 on success. 
// Returns -1 and stores NULL on error.
extern int hashtable_get(hashtable_t, int, void**);

// hashtable_remove(table, k) removes ALL instances of k
// from the table.
extern void hashtable_remove(hashtable_t, int);

// hashtable_key_exists(table, k) returns 1 if k is in
// table.
extern int hashtable_key_exists(hashtable_t, int);

// Frees the memory allocated to the given table.
extern void hashtable_free(hashtable_t);

#endif /*__HASHTABLE_H__*/
