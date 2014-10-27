/*
 *  Implementation of minimsgs and miniports.
 */
#include <stdlib.h>
#include <limits.h>

#include "minimsg.h"
#include "hashtable.h"
#include "interrupts.h"  //TODO: protect shared data by disabling interrupts
#include "network.h" 
#include "synch.h"

typedef enum {BOUND, UNBOUND} port_classification;

typedef struct miniport
{
    port_classification port_type;
    unsigned int bound_port_number; // 0 indicates no bound port
    unsigned int unbound_port_number;
    union {
        // bound ports need a destination address
        network_address_t destination_address;
        // while unbound ports only need a blocking semaphore
        semaphore_t available_messages_sema;
    } port_data;
}miniport; 

// The port numbers assigned to new ports.
int current_bound_port_number;

// If a port number is in unavailable_bound_ports, then
// it is currently being used.
// NOTE: access to  this table must be protected from
// interrupts!
hashtable_t unavailable_bound_ports;

// A helper function to get the next available bound port number.
// Returns -1 if no bound ports are available. XXX: possibly change
int get_next_bound_pn() {
    // We count the number of loops to ensure that we don't check for the 
    // same port number twice.
    // TODO: what to do when there are no available ports? Semaphore maybe?
    // ports are numbered 32768 - 65535
    int num_ports = 65535 - 32767;
    int num_loops = 0;
    while (num_loops < num_ports
           && hashtable_key_exists(unavailable_bound_ports, current_bound_port_number))
    {
        current_bound_port_number++;
        if (current_bound_port_number > 65535) {
            // Rollover from 65535 to 32768
            current_bound_port_number = 32768;
        }
        num_loops++;
    }
    
    if (num_loops >= num_ports) return -1; // no ports were available
    
    return current_bound_port_number; 
}

/* performs any required initialization of the minimsg layer.
 */
void
minimsg_initialize()
{
    current_bound_port_number = 32768;
    unavailable_bound_ports = hashtable_create();
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
    miniport_t new_miniport;
    semaphore_t new_available_messages_sema;

    new_miniport = (miniport_t) malloc(sizeof(miniport));

    // Port numbers outside of this range are invalid.
    if (port_number < 0 || port_number > 32767) return NULL;

    new_available_messages_sema = semaphore_create();
    semaphore_initialize(new_available_messages_sema, 0);

    new_miniport->port_type = UNBOUND;
    new_miniport->bound_port_number = 0;
    new_miniport->unbound_port_number = port_number;
    new_miniport->port_data.available_messages_sema = new_available_messages_sema; 

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

    new_miniport = (miniport_t)malloc(sizeof(miniport));
    
    new_miniport->port_type = BOUND;
    new_miniport->bound_port_number = get_next_bound_pn();
    new_miniport->unbound_port_number = remote_unbound_port_number;
    network_address_copy(addr, miniport->port_data.destination_address);

    // Make the bound port number unavailable to other miniports.
    unavailable_bound_ports = hashtable_put(unavailable_bound_ports, current_bound_port_number);

    return 0;
}

/* Destroys a miniport and frees up its resources. If the miniport was in use at
 * the time it was destroyed, subsequent behavior is undefined.
 */
void
miniport_destroy(miniport_t miniport)
{
    if (miniport->port_type == BOUND) {
        // If this was a bound port, make its bound port number available.
        hashtable_remove(unavailable_bound_ports, miniport->bound_port_number);
        // XXX: what do to about miniport->data.destination_address?
    } else {
        // If this was an unbound port, free its semaphore.
        semaphore_destroy(miniport->port_data.available_messages_sema);
    }  

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

