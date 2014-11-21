#include <stdlib.h>

#include "miniheader.h"
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

//Current request id.
static int current_request_id = 0;

//Hash table to track semaphores for route replies.
//Indexed by the hash of the destination address the reply is coming from.
semaphore_t reply_sema;


//Pack/unpack routing headers.
routing_header_t pack_routing_header(char pkt_type, network_address_t dest_address, int id, 
									 int ttl, path_t path){
	routing_header_t rheader;

	rheader = (routing_header_t) malloc(sizeof(struct routing_header));
	rheader->routing_packet_type = pkt_type;
	pack_address(rheader->destination, dest_address);
	pack_unsigned_int(rheader->id, id);
	pack_unsigned_int(rheader->ttl, ttl);
	pack_unsigned_int(rheader->path_len, path->len);

	for(i = 0; i < path->len; i++){
		network_address_t hostaddr;
		unpack_address(path->hlist[i], hostaddr);

		pack_address(rheader->path[i], hostaddr);
	}

	return rheader;
}

void unpack_routing_header(routing_header_t rheader, char *pkt_type, network_address_t dest_address, int *id, 
									 int *ttl, path_t path){
	*pkt_type = rheader->routing_packet_type;
	unpack_address(rheader->destination, dest_address);
	*id = unpack_unsigned_int(rheader->id);
	*ttl = unpack_unsigned_int(rheader->ttl);

	path->len = unpack_unsigned_int(rheader->path_len);
	for(int i = 0; i < path->len; i++){
		unpack_address(rheader->path[i], path->hlist[i]);
	}
}



void route_expiration_handler(void* dest_address){

}

//Used by either the original host to find
//routes to the passed in host through bcast. Blocking call.
void discover_route_to(network_address_t dest_address,){
	// routing_header_t rheader;
	// char pkt_type;
	// network_address_t dest_address;
	// int id;
	// int ttl; 
	// path_t path;


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
	new_path = (path_t) malloc(sizeof(struct path));
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

	unpack_routing_header(header, &pkt_type, dest_address, &id, &ttl, path);

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
void data_route_to(network_address_t dest_address, char *packet, int packet_len){
	path_t path;
	routing_header_t header;

	// Get the path from the route table and use it to construct the header.
	hashtable_get(route_table, hash_address(dest_address), &path);
	header = pack_routing_header(ROUTING_DATA, dest_address, 0, 0, path);

	// We send the data packet to the first node in the path, which is at index 1;
	network_send_pkt(path[1], sizeof(struct routing_header), routing_header, packet_len, packet);
}

//Used by the intermediary hosts further unicast send a data packet to the endpoint.
int data_route_fwd_to(network_address_t dest_address){
	return 1;
}


/* Performs any initialization of the miniroute layer, if required. */
void miniroute_initialize()
{
	route_table = hashtable_create();

	route_table_access_sema = semaphore_create();
	semaphore_initialize(route_table_access_sema, 1);

	reply_sema_table = hashtable_create();
}

/*
* Performs the unwrapping of the raw_pkt and handles it accordingly, calling one of the functions above.
*/
int miniroute_route_pkt(network_interrupt_arg_t *raw_pkt, network_interrupt_arg_t *data_pkt){
		routing_header_t rheader;
		char pkt_type;
		network_address_t dest_address;
		int id;
		int ttl; 
		path_t path;

		if(raw_pkt == NULL || raw_pkt->size < sizeof(struct routing_header)) return 0;

		rheader = (routing_header_t) raw_pkt->buffer;
		unpack_routing_header(rheader, &pkt_type, dest_address, &id, &ttl, path);

		if(pkt_type == ROUTING_DATA){
			int fwd_result = 0;
			fwd_result = data_route_fwd_to(dest_address);

			if(fwd_result){
				//Copy raw_pkt with network header ripped off.
				data_pkt = (network_interrupt_arg_t *) malloc(sizeof(network_interrupt_arg_t));
				network_address_copy(raw_pkt->sender, data_pkt->sender);
				data_pkt->size = raw_pkt->size - sizeof(struct routing_header);
				memcpy(data_pkt->buffer, &raw_pkt->buffer[sizeof(struct routing_header)], data_pkt->size);
			}

			return fwd_result;
		}
		if(pkt_type == ROUTING_ROUTE_DISCOVERY){
			discover_route_fwd_to(dest_address, rheader);
			return 0;
		}
		if(pkt_type == ROUTING_ROUTE_REPLY){
			reply_route_fwd_to(dest_address, rheader);
			return 0;
		} 
		
		return 0;
}

void miniroute_network_handler(network_interrupt_arg_t *raw_pkt){
	network_interrupt_arg_t *data_pkt;
	char protocol;
	int handle_payload = 0;
	
	handle_payload = miniroute_route_pkt(raw_pkt, data_pkt);

	if(!handle_payload || data_pkt == NULL || data_pkt->size == 0) return;

	protocol = data_pkt->buffer[0];
	if(protocol == PROTOCOL_MINIDATAGRAM){
		minimsg_dropoff_message(data_pkt);
		return;
	}
	if(protocol == PROTOCOL_MINISTREAM){
		minisocket_dropoff_packet(data_pkt);
		return;
	}

}


/* sends a miniroute packet, automatically discovering the path if necessary. See description in the
 * .h file.
 */
int miniroute_send_pkt(network_address_t dest_address, int hdr_len, char* hdr, int data_len, char* data)
{
	//Add network hdr in front of the hdr.


	return network_send_pkt(dest_address,hdr_len, hdr,data_len,data);

	// path_t src_dst_path;

	// if(hashtable_get(route_table,hash_address(dest_address),(void **) &src_dst_path) != 0){
	// 	//No fresh route was found, so trigger the route discovery protocol.
	// 	semaphore_P(route_table_access_sema);


	// 	semaphore_V(route_table_access_sema);
	// }


	// return 0;

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