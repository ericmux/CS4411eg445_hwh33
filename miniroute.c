#include "miniroute.h"
#include "hashtable.h"
#include "alarm.h"
#include "synch.h"


struct path {
	int len;
	char hlist[MAX_ROUTE_LENGTH][8];
}; 
typedef struct path *path_t;

typedef struct routing_header *routing_header_t;

//Hash table to keep track of the most recent routes.
hashtable_t route_table;

//Semaphore for mutual exclusion on the access to the route table.
semaphore_t route_table_acess_sema;

void route_expiration_handler(void* dest_address){

}

//Used by either the original caller host to find
//routes to the passed in host through bcast. Blocking call.
void discover_route_to(network_address_t dest_address){

}

//Used by intermediary hosts further bcast until the endhost is reached.
void discover_route_fwd_to(network_address_t dest_address){

}

//Used by the endpoint to send a unicast packet back to the source.
void reply_route_to(network_address_t src_address){

}


//Used by the intermediary hosts to 
void 



void miniroute_route_pkt(network_interrupt_arg_t *raw_pkt, network_interrupt_arg_t *data_pkt){

}




/* Performs any initialization of the miniroute layer, if required. */
void miniroute_initialize()
{
	route_table = hashtable_create();

	route_access_semas = semaphore_create();
	semaphore_initialize(route_access_sema, 1);
}

/* sends a miniroute packet, automatically discovering the path if necessary. See description in the
 * .h file.
 */
int miniroute_send_pkt(network_address_t dest_address, int hdr_len, char* hdr, int data_len, char* data)
{
	path_t src_dst_path;
	semaphore_t route_access_sema;

	if(hashtable_get(route_table,hash_address(dest_address),(void **) &src_dst_path) != 0){
		//No fresh route was found, so trigger the route discovery protocol.
		semaphore_P(route_access_sema);


		semaphore_V(route_access_sema);
	}


	return 0;

}

/* hashes a network_address_t into a 16 bit unsigned int */
unsigned short hash_address(network_address_t address)
{
	unsigned int result = 0;
	int counter;

	for (counter = 0; counter < 3; counter++)
		result ^= ((unsigned short*)address)[counter];

	return result % 65521;
}