#include <stdlib.h>

#include "miniheader.h"
#include "miniroute.h"
#include "minisocket.h"
#include "minimsg.h"
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
	int i;

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
									 int *ttl, path_t *path){
	int i;
	path_t p;

	*pkt_type = rheader->routing_packet_type;
	unpack_address(rheader->destination, dest_address);
	*id = unpack_unsigned_int(rheader->id);
	*ttl = unpack_unsigned_int(rheader->ttl);

	*path = (path_t) malloc(sizeof(struct path));
	p = *path;

	p->len = unpack_unsigned_int(rheader->path_len);
	for(i = 0; i < p->len; i++){
		network_address_t addr;
		unpack_address(rheader->path[i], addr);
		pack_address(p->hlist[i], addr);
	}
}



void route_expiration_handler(void* dest_address){

}


//Used by the endpoint to send a unicast packet back to the source.
//discovery_path is the path the discovery packet took to this node.
void reply_route_to(network_address_t src_address, path_t discovery_path, int id){
	int i, j;
	path_t new_path;
	routing_header_t header;
	network_address_t dest_address;
	network_address_t dummy;

	// Reverse the path.
	new_path = (path_t) malloc(sizeof(struct path));
	for (i = discovery_path->len-1, j = 0; i >= 0; i--, j++) {
		unpack_address(discovery_path->hlist[i], dummy);
		pack_address(new_path->hlist[j], dummy);
	}

	header = pack_routing_header(ROUTING_ROUTE_REPLY, src_address, id, 0, new_path);

	// Now we send the packet to the first node in the path, which is at index 1.
	unpack_address(new_path->hlist[1], dest_address);
	network_send_pkt(dest_address, sizeof(struct routing_header), (char *) header, MAX_ROUTE_LENGTH, NULL);
}


//Used by the intermediary hosts to further unicast until the origin host is reached.
void reply_route_fwd_to(routing_header_t header){
	char pkt_type;
	network_address_t dest_address;
	int id;
	int ttl;
	path_t path;
	network_address_t my_address;
	interrupt_level_t old_level;
	int i;
	int next_node_idx;
	network_address_t dummy;
	network_address_t source_address;

	unpack_routing_header(header, &pkt_type, dest_address, &id, &ttl, &path);

	// Check if we are the intended destination of this reply.
	network_get_my_address(my_address);
	if (network_compare_network_addresses(my_address, dest_address)) {
		
		old_level = set_interrupt_level(DISABLED);

		// If we have already received a reply to this discovery, then drop the packet.
		if (current_request_id > id) {
			set_interrupt_level(old_level);
			return;
		}

		// We have received a reply to our discovery. Put the path in the route table,
		// V the appropriate semaphore, and return.

		// Get the end of the route (the first entry in the path) and put the route in
		// the hash table.
		pack_address(path->hlist[0], source_address);
		route_table = hashtable_put(route_table, hash_address(source_address), path);
		
		// TODO: set an alarm to remove the path from the table when it becomes stale.
		
		current_request_id++;
		semaphore_V(reply_sema);
		set_interrupt_level(old_level);
		return; 
	}
	
	// If we are not the target, we decrement ttl. If ttl == 0, then the packet 
	// has exceeded its time to live, so we drop it. This shouldn't happen in 
	// replies, but we include this check for sanity purposes.
	ttl--;
	if (ttl == 0) return;

	// Now we need to find the next node in the path. This is simply the node
	// after the node corresponding to ourselves.
	next_node_idx = -1;
	for (i = 0; i < path->len; i++) {
		unpack_address(path->hlist[i], dummy);
		if (network_compare_network_addresses(my_address, dummy)) {
			next_node_idx = i + 1;
		}
	}

	// Now we pack the header with the new ttl and forward to the next node.
	header = pack_routing_header(pkt_type, dest_address, id, ttl, path);
	unpack_address(path->hlist[next_node_idx], dest_address);
	network_send_pkt(dest_address, sizeof(struct routing_header), (char *) header, 0, NULL);
}

//Used by the original host send a data packet to the endpoint.
void data_route_to(network_address_t dest_address, char *packet, int packet_len){
	path_t path;
	routing_header_t header;

	// Get the path from the route table and use it to construct the header.
	hashtable_get(route_table, hash_address(dest_address), (void **) &path);
	header = pack_routing_header(ROUTING_DATA, dest_address, 0, 0, path);

	// We send the data packet to the first node in the path, which is at index 1;
	unpack_address(path->hlist[1], dest_address);
	network_send_pkt(dest_address, sizeof(struct routing_header), (char *) header, packet_len, packet);
}

//Used by the intermediary hosts further unicast send a data packet to the endpoint.
//Returns 1 if this node is the inteded destination and 0 otherwise.
int data_route_fwd_to(routing_header_t header, char *packet, int packet_len){
	char pkt_type;
	network_address_t dest_address;
	int id;
	int ttl;
	path_t path;
	network_address_t my_address;
	int i;
	int next_node_idx;
	network_address_t dummy;

	path = NULL;

	unpack_routing_header(header, &pkt_type, dest_address, &id, &ttl, &path);

	// Check if we are the intended destination of this packet, and if so return 1.
	network_get_my_address(my_address);
	if (network_compare_network_addresses(my_address, dest_address)) {
		return 1;
	}

	// If we are not the target, we decrement ttl. If ttl == 0, then the packet 
	// has exceeded its time to live, so we drop it. 
	ttl--;
	if (ttl == 0) return 0;

	// Now we need to find the next node in the path. This is simply the node
	// after the node corresponding to ourselves.
	next_node_idx = -1;
	for (i = 0; i < path->len; i++) {
		unpack_address(path->hlist[i], dummy);
		if (network_compare_network_addresses(my_address, dummy)) {
			next_node_idx = i + 1;
		}
	}

	// Now we pack the header with the new ttl and forward to the next node.
	header = pack_routing_header(pkt_type, dest_address, id, ttl, path);
	unpack_address(path->hlist[next_node_idx], dest_address);
	network_send_pkt(dest_address, sizeof(struct routing_header), (char *) header, packet_len, packet);

	return 0;
}

//Used by the original host to find
//routes to the passed in host through bcast. Blocking call.
void discover_route_to(network_address_t dest_address){
	routing_header_t header;
	path_t path;
	void *dummy_ptr;
	network_address_t my_address;

	// It's possible that the call to discover_route_to was made shortly after a
	// route to the destination was discovered by another thread on this machine.
	// In this case, we just return.
	if (hashtable_get(route_table, hash_address(dest_address), (void **) &dummy_ptr) == 0) {
		return;
	}

	// We initialize the path with an array with only one valid entry; our own address.
	network_get_my_address(my_address);
	path = (path_t) malloc(sizeof(struct path));
	path->len = 1;
	pack_address(path->hlist[0], my_address);

	// Send a broadcast out on the network and wait for a reply by calling P on the
	// reply semaphore.
	header = pack_routing_header(
		ROUTING_ROUTE_DISCOVERY, dest_address, current_request_id, MAX_ROUTE_LENGTH, path);

	network_bcast_pkt(sizeof(struct routing_header), (char *) header, 0, NULL);

	// TODO: Set an alarm to wake us up in the case of discovery timeout.

	semaphore_P(reply_sema);

	// When we come back from the reply semaphore, either the timeout alarm has gone
	// off or the route has been put into the route table. Either way, just return.
	// XXX: shouldn't we return some signal upon failure?
	return;
}

//Used by intermediary hosts further bcast until the endhost is reached.
void discover_route_fwd_to(routing_header_t header){
	char pkt_type;
	network_address_t dest_address;
	int id;
	int ttl;
	path_t path;
	network_address_t my_address;
	network_address_t source_address;

	unpack_routing_header(header, &pkt_type, dest_address, &id, &ttl, &path);

	// Add ourselves to the path.
	network_get_my_address(my_address);
	pack_address(path->hlist[path->len], my_address);
	path->len++;

	// Check if we are the intended target of this discovery.
	if (network_compare_network_addresses(my_address, dest_address)) {
		// If we are the intended target then we need to send a reply to the source.
		// The source is the first address in the path.
		unpack_address(path->hlist[0], source_address);
		reply_route_to(source_address, path, id);
	}

	// We decrement ttl. If ttl == 0, then the packet has exceeded its time to live, 
	// so we drop it.
	ttl--;
	if (ttl == 0) return;

	// If the packet still has hops to make, we broadcast it.
	// XXX: bouning discovery packets??
	header = pack_routing_header(ROUTING_ROUTE_DISCOVERY, dest_address, id, ttl, path);
	network_bcast_pkt(sizeof(struct routing_header), (char *) header, 0, NULL);
}


/* Performs any initialization of the miniroute layer, if required. */
void miniroute_initialize()
{
	route_table = hashtable_create();

	route_table_access_sema = semaphore_create();
	semaphore_initialize(route_table_access_sema, 1);

	reply_sema = semaphore_create();
	semaphore_initialize(reply_sema, 0);
}

/*
* Performs the unwrapping of the raw_pkt and handles it accordingly, calling one of the functions above.
*/
int miniroute_route_pkt(network_interrupt_arg_t *raw_pkt, network_interrupt_arg_t **data_pkt_ptr){
		routing_header_t rheader;
		char pkt_type;
		network_address_t dest_address;
		int id;
		int ttl; 
		path_t path = NULL;

		if(raw_pkt == NULL || raw_pkt->size < sizeof(struct routing_header)) return 0;

		rheader = (routing_header_t) raw_pkt->buffer;
		unpack_routing_header(rheader, &pkt_type, dest_address, &id, &ttl, &path);

		if(pkt_type == ROUTING_DATA){
			int fwd_result = 0;
			fwd_result = data_route_fwd_to(rheader, 
										   &raw_pkt->buffer[sizeof(struct routing_header)],
										   raw_pkt->size - sizeof(struct routing_header));

			if(fwd_result){
				//Copy raw_pkt with network header ripped off.
				network_interrupt_arg_t *payload;
				payload = (network_interrupt_arg_t *) malloc(sizeof(network_interrupt_arg_t));
				network_address_copy(raw_pkt->sender, payload->sender);
				payload->size = raw_pkt->size - sizeof(struct routing_header);
				memcpy(payload->buffer, &raw_pkt->buffer[sizeof(struct routing_header)], payload->size);

				*data_pkt_ptr = payload;

				free(raw_pkt);
			}

			return fwd_result;
		}
		if(pkt_type == ROUTING_ROUTE_DISCOVERY){
			discover_route_fwd_to(rheader);
			return 0;
		}
		if(pkt_type == ROUTING_ROUTE_REPLY){
			reply_route_fwd_to(rheader);
			return 0;
		} 
		
		return 0;
}

void miniroute_network_handler(network_interrupt_arg_t *raw_pkt){
	network_interrupt_arg_t *data_pkt;
	char protocol;
	int handle_payload = 0;

	data_pkt = NULL;
	
	handle_payload = miniroute_route_pkt(raw_pkt, &data_pkt);

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
	routing_header_t network_hdr;
	network_address_t next_hop_addr;
	interrupt_level_t old_level;

	path_t src_dst_path;

	old_level = set_interrupt_level(DISABLED);
	if(hashtable_get(route_table,hash_address(dest_address),(void **) &src_dst_path) != 0){
		set_interrupt_level(old_level);		

		//No fresh route was found, so trigger the route discovery protocol.
		semaphore_P(route_table_access_sema);

		discover_route_to(dest_address);

		semaphore_V(route_table_access_sema);

		old_level = set_interrupt_level(DISABLED);
		if(hashtable_get(route_table,hash_address(dest_address),(void **) &src_dst_path) != 0){
			set_interrupt_level(old_level);	
			return -1;
		}

	}

	unpack_address(src_dst_path->hlist[1], next_hop_addr);

	old_level = set_interrupt_level(DISABLED);
	network_hdr = pack_routing_header(ROUTING_DATA, next_hop_addr, current_request_id, MAX_ROUTE_LENGTH, src_dst_path);
	set_interrupt_level(old_level);

	return network_send_pkt(next_hop_addr,sizeof(struct routing_header), (char *) network_hdr, data_len + hdr_len, hdr);

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