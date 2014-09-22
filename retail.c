/* retail.c
 *
 * Simulates a produce-consumer problem in the context 
 *   of a retail store unpacking phones to sell to 
 *   customers.
 * Problem is as follows:
 * - N employees are constantly unpacking phones.
 * - Each phone has a unique serial number.
 * - M customers line up to get the phones. They receive
 *     phones on a first come, first serve basis. If a
 *     phone is available, the customer receives their
 *     phone immediately, otherwise they wait.
 * - When a customer receives their phone it is "activated"
 *     by printing the serial number to stdout.
 *
*/

#include "synch.h"
#include "minithread.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define N_EMPLOYEES 10
#define M_CUSTOMERS 10
#define BUFFER_SIZE 10


//Testing constants
#define PRODUCTION_CAP 			50
#define INIT_ROUND_ROBIN 		"rr"
#define INIT_CONSUMER_BEFORE	"cb"
#define INIT_CONSUMER_AFTER 	"ca"
#define INIT_RANDOM				"rand" 

// Semaphores to guard against concurrency issues
semaphore_t space_sem;
semaphore_t phone_sem;
semaphore_t global_mutex;

// The phones are stored in a buffer by their serial number
int phone_buffer[BUFFER_SIZE];

// current_serial_number gets incremented so that the first
// phone unpacked has serial number 0, the second has serial
// number 1 and so on
int current_serial_number;

int in, out;

// The "produce" function
int unpack(int *arg) {
	int new_serial_number;
	int i = 0;

	while(i < PRODUCTION_CAP) {
	
		semaphore_P(space_sem);
	
		// "unwrap" a phone by generating a new serial number
		// and placing it in the phone buffer 
		semaphore_P(global_mutex);
		new_serial_number = current_serial_number++;
		phone_buffer[in++] = new_serial_number;
		if (in >= BUFFER_SIZE) in = 0;
		semaphore_V(global_mutex);

		semaphore_V(phone_sem);

		minithread_yield();

		i++;

	}

	return 0;
}

// The "consume" function
int purchase(int *arg) {
	int purchased_serial_number;
	int id = *arg;
	int i = 0;

	while(i < PRODUCTION_CAP) {

		semaphore_P(phone_sem);

		// "purchase" a phone by retrieving its serial number
		// from the phone buffer and printing it to stdout
		semaphore_P(global_mutex);
		purchased_serial_number = phone_buffer[out++];
		printf("Phone purchased by %d. Serial number = %i\n", id, purchased_serial_number);
		if (out >= BUFFER_SIZE) out = 0;
		semaphore_V(global_mutex);

		semaphore_V(space_sem);

		minithread_yield();

		i++;

	}

	return 0;
}

int initialize_threads(int *arg) {
	int i;
	int k;
	char *impl;
	impl = (char *) arg;

	if(impl == NULL){
		strcpy(impl, INIT_CONSUMER_AFTER);
	}

	if(strcmp(impl, INIT_CONSUMER_AFTER)){

		// start the employees working
		for (i = 0; i < N_EMPLOYEES; i++) {
			minithread_fork(unpack, NULL);
		}

		// start the customers purchasing
		for (i = 0; i< M_CUSTOMERS; i++) {
			int *p = (int *) malloc(sizeof(int));
			*p = i;
			minithread_fork(purchase, p);
		}

	} else if(strcmp(impl, INIT_CONSUMER_BEFORE)){

		// start the customers purchasing
		for (i = 0; i< M_CUSTOMERS; i++) {
			int *p = (int *) malloc(sizeof(int));
			*p = i;
			minithread_fork(purchase, p);
		}

		// start the employees working
		for (i = 0; i < N_EMPLOYEES; i++) {
			minithread_fork(unpack, NULL);
		}

	} else if(strcmp(impl, INIT_ROUND_ROBIN)){

		//round robin
		for(k = 0, i = 0; k < N_EMPLOYEES + M_CUSTOMERS; k++){
			if(k%2 == 0){
				int *p = (int *) malloc(sizeof(int));
				*p = i;
				minithread_fork(purchase, p);
				i++;		
			} else {
				minithread_fork(unpack, NULL);
			}
		}

	} else if(strcmp(impl, INIT_RANDOM)){
		//random init
	}




	return 0;
}

int main(int argc, char *argv[]) {
	space_sem = semaphore_create();
	phone_sem = semaphore_create();
	global_mutex = semaphore_create();

	semaphore_initialize(space_sem, BUFFER_SIZE);
	semaphore_initialize(phone_sem, 0);
	semaphore_initialize(global_mutex, 1);

	current_serial_number = 0;
	in = out = 0;

	printf("Initializing system...\n");
	if(argc == 1){
		minithread_system_initialize(initialize_threads, NULL);
	}
	else if(argc == 2){
		char * p;
		strcpy(p, argv[1]);

		minithread_system_initialize(initialize_threads, (int *) p);
	} else {
		printf("Too many args.");
	}

	// Should not be reachable
	return -1;
}

