/*
 *  Implementation of minimsgs and miniports.
 */
#include <stdlib.h>
#include <limits.h>

#include "minimsg.h"
#include "hashtable.h"
#include "interrupts.h"  //TODO: protect shared data by disabling interrupts
#include "network.h" 

// Both port numbers need to be at least 15 bits. We make them 16 so
// that they are word-alligned. PN = Port Number.
#define BITS_PER_PORT_NUMBER 16
// By dimensional analysis: (bits / PN) * (bytes / bit) = (bytes / PN)
#define BYTES_PER_PN_BUFFER (BITS_PER_PORT_NUMBER / CHAR_BIT)

// XXX: is changing this to a typedef okay?
typedef struct miniport
{
    char unbound_pn_buffer[BYTES_PER_PN_BUFFER];
    char bound_pn_buffer[BYTES_PER_PN_BUFFER];  // -1 indicates no bound port
    char dest_addr_buffer[sizeof(network_address_t)]; //XXX: is this right?..
}miniport; 

// The port numbers assigned to new ports.
int current_bound_port_number;

// If a port number is in bound_port_table, then
// it is currently being used.
// NOTE: access to  this table must be protected from
// interrupts!
hashtable_t bound_port_table;

// A helper function to get an int from a char buffer.
int int_from_buffer(char *buffer) {
    return *(int *)buffer;
}

// A helper function to get a char buffer from an int.
char* buffer_from_int(int n) {
    char *new_buffer = (char *)malloc(sizeof(int));
    new_buffer = (char *)&n;
    return new_buffer;
}

// A helper function to get a network_address_t from a char buffer.
// We have to return a pointer to a network_address_t because
// we cannot return a type defined as an array.
network_address_t* addr_from_buffer(char *buffer) {
    network_address_t *address_ptr = (network_address_t *)malloc(sizeof(network_address_t));
    network_address_copy((unsigned int *)buffer, *address_ptr);
    return address_ptr;
}

// A helper function to get a char buffer from a network_addr_t.
// XXX: Passing in address as a void pointer because of compile
// error. Fix this. (should be using network_address_t)
char* buffer_from_addr(void *address) {
    char *new_buffer = (char *)malloc(sizeof(network_address_t));
    network_address_copy((network_address_t) address, (network_address_t) new_buffer);
    //new_buffer = (char *)&network_address_t;
    return new_buffer;
}

/* performs any required initialization of the minimsg layer.
 */
void
minimsg_initialize()
{
    current_bound_port_number = 32768;
    bound_port_table = hashtable_create();
}

/* Creates an unbound port for listening. Multiple requests to create the same
 * unbound port should return the same miniport reference. It is the responsibility
 * of the programmer to make sure he does not destroy unbound miniports while they
 * are still in use by other threads -- this would result in undefined behavior.
 * Unbound ports must range from 0 to 32767. If the programmer specifies a port number
 * outside this range, it is considered an error.
 */
miniport_t
miniport_create_unbound(int port_number)
{
    miniport_t new_miniport = (miniport_t) malloc(sizeof(miniport));

    // Port numbers outside of this range are invalid.
    if (port_number < 0 || port_number > 32767) return NULL;

    //new_miniport->unbound_pn_buffer = buffer_from_int(port_number);
    // We are just setting this port up for listening, so the following are NULL.
    // We store -1 as the bound port number to avoid bad data.
    //new_miniport->bound_pn_buffer = buffer_from_int(-1);
    //new_miniport->dest_addr_buffer = NULL;

    return new_miniport;
}

/* Creates a bound port for use in sending packets. The two parameters, addr and
 * remote_unbound_port_number together specify the remote's listening endpoint.
 * This function should assign bound port numbers incrementally between the range
 * 32768 to 65535. Port numbers should not be reused even if they have been destroyed,
 * unless an overflow occurs (ie. going over the 65535 limit) in which case you should
 * wrap around to 32768 again, incrementally assigning port numbers that are not
 * currently in use.
 */
miniport_t
miniport_create_bound(network_address_t addr, int remote_unbound_port_number)
{
    miniport_t new_miniport;

    // Obtain the port number for the bound port by searching for the first
    // unused port.
    // TODO: what to do when there are no available ports?
    while (hashtable_key_exists(bound_port_table, current_bound_port_number)) {
        current_bound_port_number++;
    }

    new_miniport = (miniport_t)malloc(sizeof(miniport));

    // Set up our destination address and port number.
    //new_miniport->unbound_pn_buffer = buffer_from_int(remote_unbound_port_number);
    //new_miniport->dest_addr_buffer = buffer_from_addr(addr);
    // Set the bound port to use for sending.
    //new_miniport->bound_pn_buffer = buffer_from_int(current_bound_port_number);

    // Now we need to add the bound port number to the table so that no other
    // miniports try to take it.
    bound_port_table = hashtable_put(bound_port_table, current_bound_port_number);

    // The last thing we do is increment the bound port counter, making sure to
    // roll over if overflow occurs.
    current_bound_port_number++;
    if (current_bound_port_number > 65535) current_bound_port_number = 32768;

    return 0;
}

/* Destroys a miniport and frees up its resources. If the miniport was in use at
 * the time it was destroyed, subsequent behavior is undefined.
 */
void
miniport_destroy(miniport_t miniport)
{
    int bound_port_number;
    
    // If this miniport had a bound port, then we make that port available by 
    // removing it from the table.
    bound_port_number = int_from_buffer(miniport->bound_pn_buffer);
    if (bound_port_number != -1) {
        hashtable_remove(bound_port_table, bound_port_number);
    }

    // Now we simply free the miniport's buffers, then the miniport itself.
    free(miniport->bound_pn_buffer);
    free(miniport->unbound_pn_buffer);
    free(miniport->dest_addr_buffer);

    free(miniport);
}

/* Sends a message through a locally bound port (the bound port already has an associated
 * receiver address so it is sufficient to just supply the bound port number). In order
 * for the remote system to correctly create a bound port for replies back to the sending
 * system, it needs to know the sender's listening port (specified by local_unbound_port).
 * The msg parameter is a pointer to a data payload that the user wishes to send and does not
 * include a network header; your implementation of minimsg_send must construct the header
 * before calling network_send_pkt(). The return value of this function is the number of
 * data payload bytes sent not inclusive of the header.
 */
int
minimsg_send(miniport_t local_unbound_port, miniport_t local_bound_port, minimsg_t msg, int len)
{
    return 0;
}

/* Receives a message through a locally unbound port. Threads that call this function are
 * blocked until a message arrives. Upon arrival of each message, the function must create
 * a new bound port that targets the sender's address and listening port, so that use of
 * this created bound port results in replying directly back to the sender. It is the
 * responsibility of this function to strip off and parse the header before returning the
 * data payload and data length via the respective msg and len parameter. The return value
 * of this function is the number of data payload bytes received not inclusive of the header.
 */
int minimsg_receive(miniport_t local_unbound_port, miniport_t* new_local_bound_port, minimsg_t msg, int *len)
{
    return 0;
}

