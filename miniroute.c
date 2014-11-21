#include <stdlib.h>

#include "miniroute.h"
#include "hashtable.h"
#include "alarm.h"
#include "synch.h"
#include "interrupts.h"


struct path {
	int len;
	char hlist[MAX_ROUTE_LENGTH][8];
}; 
typedef struct path *path_t;

typedef struct routing_header *routing_header_t;

//Hash table to keep track of the most recent routes.
hashtable_t route_table;

//Semaphore for mutual exclusion on the access to the route table.
semaphore_t route_table_access_sema;

//Hash table to track semaphores for route replies.
//Indexed by the hash of the destination address the reply is coming from.
hashtable_t reply_sema_table;


//Pack/unpack routing headers.
routing_header_t pack_routing_header(char pkt_type, network_address_t dest_address, int id, 
									 int ttl, path_t path){
	return NULL;
}

void unpack_routing_header(char *pkt_type, network_address_t dest_address, int *id, 
									 int *ttl, path_t path){

}




void route_expiration_handler(void* dest_address){

}

//Used by either the original host to find
//routes to the passed in host through bcast. Blocking call.
void discover_route_to(network_address_t dest_address){

}

//Used by intermediary hosts further bcast until the endhost is reached.
void discover_route_fwd_to(network_address_t dest_address){

}

//Used by the endpoint to send a unicast packet back to the source.
//discovery_path is the path the discovery packet took to this node.
void reply_route_to(network_address_t src_address, path_t discovery_path, int id){
	int i, j;
	path_t new_path;
	routing_header_t header;

	// Reverse the path.
	new_path = (path_t) malloc(sizeof(path));
	for (i = discovery_path->len-1, j = 0; i >= 0; i--, j++) {
		new_path->hlist[j] = discovery_path->hlist[i];
	}

	header = pack_routing_header(ROUTING_ROUTE_REPLY, src_address, id, 0, new_path);

	// Now we send the packet to the first node in the path, which is at index 1.
	network_send_pkt(new_path[1], sizeof(struct routing_header), header, 0, NULL);
}


//Used by the intermediary hosts to further unicast until the origin host is reached.
void reply_route_fwd_to(network_address_t src_address, routing_header_t header){
	char pkt_type;
	network_address_t dest_address;
	int id;
	int ttl;
	path_t path;
	network_address_t my_address;
	semaphore_t reply_sema;
	network_address_t reply_source;
	interrupt_level_t old_level;

	unpack_routing_header(&pkt_type, dest_address, &id, &ttl, path);

	// Check if we are the intended destination of this reply.
	network_get_my_address(my_address);
	if (network_compare_network_addresses(my_address, dest_address)) {
		
		old_level = set_interrupt_level(DISABLED);

		// If we have already received a reply to this discovery, then drop the packet.
		if (current_request_id > id) {
			set_interrupt_level(old_level);
			return;
		}

		// We have received a reply to our discovery. V the appropriate semaphore
		// and return.
		current_request_id++;
		network_address_copy(path->hlist[0], reply_source);
		hashtable_get(reply_sema_table, hash_address(reply_source), *reply_sema);
		semaphore_V(reply_sema);
		set_interrupt_level(old_level);
		return; 
	}
	
	// If we are not the target, we increment ttl and forward the
	// packet to the next node in the path, which is at index ttl+1.
	ttl++;
	header = pack_routing_header(pkt_type, dest_address, id, ttl, path);

	network_send_pkt(path[ttl+1], sizeof(struct routing_header), header, 0, NULL);
}

//Used by the original host send a data packet to the endpoint.
void data_route_to(network_address_t dest_address){

}

//Used by the intermediary hosts further unicast send a data packet to the endpoint.
void data_route_fwd_to(network_address_t dest_address){

}


/*
* Performs the unwrapping of the raw_pkt and handles it accordingly, calling one of the functions above.
*/
void miniroute_route_pkt(network_interrupt_arg_t *raw_pkt, network_interrupt_arg_t *data_pkt){

}




/* Performs any initialization of the miniroute layer, if required. */
void miniroute_initialize()
{
	route_table = hashtable_create();

	route_table_access_sema = semaphore_create();
	semaphore_initialize(route_table_access_sema, 1);
}

/* sends a miniroute packet, automatically discovering the path if necessary. See description in the
 * .h file.
 */
int miniroute_send_pkt(network_address_t dest_address, int hdr_len, char* hdr, int data_len, char* data)
{
	path_t src_dst_path;

	if(hashtable_get(route_table,hash_address(dest_address),(void **) &src_dst_path) != 0){
		//No fresh route was found, so trigger the route discovery protocol.
		semaphore_P(route_table_access_sema);


		semaphore_V(route_table_access_sema);
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