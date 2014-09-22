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

#include <stdio.h>

#define N_EMPLOYEES 10
#define M_CUSTOMERS 10000
#define BUFFER_SIZE 10

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

	while(1) {
	
		semaphore_P(space_sem);
	
		// "unwrap" a phone by generating a new serial number
		// and placing it in the phone buffer 
		semaphore_P(global_mutex);
		new_serial_number = current_serial_number++;
		phone_buffer[in++] = new_serial_number;
		if (in >= BUFFER_SIZE) in = 0;
		semaphore_V(global_mutex);

		semaphore_V(phone_sem);

		// if more phones have been unpacked than there are
		// customers, then the employee can stop working	
		if (new_serial_number >= M_CUSTOMERS) {
			return 0;	
		}

		minithread_yield();

	}

	return 0;
}

// The "consume" function
int purchase(int *arg) {
	int purchased_serial_number;

	//while(1) {

		semaphore_P(phone_sem);

		// "purchase" a phone by retrieving its serial number
		// from the phone buffer and printing it to stdout
		semaphore_P(global_mutex);
		purchased_serial_number = phone_buffer[out++];
		printf("Phone purchased. Serial number = %i\n", purchased_serial_number);
		if (out >= BUFFER_SIZE) out = 0;
		semaphore_V(global_mutex);

		semaphore_V(space_sem);

	//	minithread_yield();

	//}

	return 0;
}

int initialize_threads(int *arg) {
	int i;

	// start the employees working
	for (i = 0; i < N_EMPLOYEES; i++) {
		minithread_fork(unpack, NULL);
	}

	// start the customers purchasing
	for (i = 0; i< M_CUSTOMERS; i++) {
		minithread_fork(purchase, NULL);
	}

	return 0;
}

int main() {
	space_sem = semaphore_create();
	phone_sem = semaphore_create();
	global_mutex = semaphore_create();

	semaphore_initialize(space_sem, BUFFER_SIZE);
	semaphore_initialize(phone_sem, 0);
	semaphore_initialize(global_mutex, 1);

	current_serial_number = 0;
	in = out = 0;

	printf("Initializing system...\n");
	minithread_system_initialize(initialize_threads, NULL);

	// Should not be reachable
	return -1;
}

