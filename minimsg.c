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
#include "queue.h"
#include "miniheader.h"

typedef enum {BOUND, UNBOUND} port_classification;

typedef struct mailbox {
    int port_number;
    semaphore_t available_messages_sema;
    queue_t received_messages;
} mailbox;

typedef struct destination_data {
    network_address_t destination_address;
    int destination_port;
    int source_port;
} destination_data;

typedef struct miniport
{
    port_classification port_type;
    union {
        // bound ports need destination data
        destination_data *destination_data;
        // while unbound ports need a mailbox
        mailbox *mailbox;
    } port_data;
} miniport; 

// The port numbers assigned to new ports.
int current_bound_port_number;

// The port tables map port numbers to miniport_t's.
// If a port number is in bound_ports_table, then
// it is currently being used.
// NOTE: access to these tables must be protected from
// interrupts!
hashtable_t bound_ports_table;
hashtable_t unbound_ports_table;

// A helper function to get the next available bound port number.
// Returns -1 if no bound ports are available. XXX: possibly change
// TODO: disable interrupts here, right?
int get_next_bound_pn() {
    // We count the number of loops to ensure that we don't check for the 
    // same port number twice.
    // TODO: what to do when there are no available ports? Semaphore maybe?
    // ports are numbered 32768 - 65535
    int num_ports = 65535 - 32767;
    int num_loops = 0;
    while (num_loops < num_ports
           && hashtable_key_exists(bound_ports_table, current_bound_port_number))
    {
        current_bound_port_number++;
        if (current_bound_port_number > 65535) {
            // Rollover from 65535 to 32768
            current_bound_port_number = 32768;
        }
        num_loops++;
    }
    
    if (num_loops >= num_ports) {return -1;} // no ports were available
    
    return current_bound_port_number; 
}

// Pack a mini_header and return the mini_header_t
// Protocol is assumed to be PROTOCOL_MINIDATAGRAM.
// All port numbers are assumed to be valid.
mini_header_t pack_header(network_address_t source_address, int source_port,
                          network_address_t dest_address, int dest_port)
{
    mini_header_t new_header = (mini_header_t)malloc(sizeof(struct mini_header));

    new_header->protocol = PROTOCOL_MINIDATAGRAM;
    pack_address(new_header->source_address, source_address);
    pack_unsigned_short(new_header->source_port, (short) source_port);
    pack_address(new_header->destination_address, dest_address);
    pack_unsigned_short(new_header->destination_port, (short) dest_port);

    return new_header;
}

//Takes in a packet as a char buffer and returns the source port.
int get_source_port(char *packet_buffer) {
// The source port is located after the protocol (a char)
// and the source address (an 8-byte char buffer).
    return (int) unpack_unsigned_short(&packet_buffer[9]);
} 

//Takes in a packet as a char buffer and returns the destination port.
int get_destination_port(char *packet_buffer) {
// The source port is located after the protocol (a char)
// and the source address (an 8-byte char buffer).
    return (int) unpack_unsigned_short(&packet_buffer[19]);
}

// Takes in a packet as a char buffer and returns the packet's payload.
minimsg_t get_payload(char *packet_buffer) {
    // The payload is located after the entire header, which
    // is a struct of type mini_header.
    minimsg_t payload_ptr = (minimsg_t) &packet_buffer[sizeof(struct mini_header)];
    return payload_ptr;
}

/* performs any required initialization of the minimsg layer.
 */
void
minimsg_initialize()
{
    current_bound_port_number = 32768;
    bound_ports_table = hashtable_create();
    unbound_ports_table = hashtable_create();
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
    mailbox *new_mailbox;
    semaphore_t new_available_messages_sema;
    queue_t new_received_messages_q;
    interrupt_level_t old_level;

    // Check for a bad port_number. Port numbers outside of this range are invalid.
    if (port_number < 0 || port_number > 32767) return NULL;

    old_level = set_interrupt_level(DISABLED);

    // Check if a miniport at this port number has already been created. If so,
    // return that miniport. hashtable_get returns 0 on success and stores the
    // pointer found in the table at the address of the 3rd argument.
    if (hashtable_get(unbound_ports_table, port_number, (void **) &new_miniport) == 0) {
        set_interrupt_level(old_level);
        return new_miniport; // it's not actually new
    }

    // If we're here, then we need to create a new miniport.
    // First thing we do is set up the new miniport's mailbox.
    new_available_messages_sema = semaphore_create();
    semaphore_initialize(new_available_messages_sema, 0);
    new_received_messages_q = queue_new();
    
    new_mailbox = (mailbox *)malloc(sizeof(mailbox));
    new_mailbox->port_number = port_number;
    new_mailbox->available_messages_sema = new_available_messages_sema;
    new_mailbox->received_messages = new_received_messages_q;

    // Now we initialize the new miniport.
    new_miniport = (miniport_t) malloc(sizeof(miniport));
    new_miniport->port_type = UNBOUND;
    new_miniport->port_data.mailbox = new_mailbox; 

    // Before we return, we store the new miniport in the unbound port table.
    unbound_ports_table = hashtable_put(unbound_ports_table, port_number, new_miniport); 

    set_interrupt_level(old_level);

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
    destination_data *new_destination_data;
    int bound_port_number;
    interrupt_level_t old_level;

    // Check for NULL input or a bad remote_unbound_port_number.
    if (addr == NULL 
        || remote_unbound_port_number < 0 || remote_unbound_port_number > 32767) {
        return NULL;
    }
  
    old_level = set_interrupt_level(DISABLED);

    // The first thing we do is set up the port's destination data.
    new_destination_data = (destination_data *)malloc(sizeof(destination_data));
    network_address_copy(addr, new_destination_data->destination_address);
    new_destination_data->destination_port = remote_unbound_port_number;

    // Now we initialize the new miniport.
    new_miniport = (miniport_t)malloc(sizeof(miniport));
    new_miniport->port_type = BOUND;
    new_miniport->port_data.destination_data = new_destination_data;

    // Before we return, we get a bound port number and put the port in the bound port table.
    // This also makes the port number unavailable to other miniports.
    bound_port_number = get_next_bound_pn();
    bound_ports_table = hashtable_put(bound_ports_table, bound_port_number, new_miniport);

    set_interrupt_level(old_level);

    return new_miniport;
}

/* Destroys a miniport and frees up its resources. If the miniport was in use at
 * the time it was destroyed, subsequent behavior is undefined.
 */
void
miniport_destroy(miniport_t miniport)
{
    interrupt_level_t old_level;

    // Check for NULL input.
    if (miniport == NULL) return;

    old_level = set_interrupt_level(DISABLED);

    if (miniport->port_type == UNBOUND) {
        hashtable_remove(unbound_ports_table, miniport->port_data.mailbox->port_number);
        set_interrupt_level(old_level);
        semaphore_destroy(miniport->port_data.mailbox->available_messages_sema);
        queue_free(miniport->port_data.mailbox->received_messages);
        free(miniport->port_data.mailbox);
    } else {
        hashtable_remove(bound_ports_table, miniport->port_data.destination_data->source_port);
        set_interrupt_level(old_level);
        free(miniport->port_data.destination_data);
    }  

    free(miniport);

    //set_interrupt_level(old_level);

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
    mini_header_t msg_header;
    network_address_t source_address;
    network_address_t destination_address;
    int source_port;
    int destination_port;
    int bytes_sent;

    // Check for NULL inputs.
    if (local_unbound_port == NULL || local_bound_port == NULL || msg == NULL) {
        return 0;
    }

    // If any of the input arguments are incorrect, the message does not get sent.
    if (local_unbound_port->port_type != UNBOUND 
        || local_bound_port->port_type != BOUND
        || len > MAX_NETWORK_PKT_SIZE) { //XXX or should we send as much as we can?
        return 0;
    }

    // Prepare the data for our header.
    network_get_my_address(source_address);
    network_address_copy(
        local_bound_port->port_data.destination_data->destination_address,
        destination_address);
    source_port = local_unbound_port->port_data.mailbox->port_number;
    destination_port = 
        local_bound_port->port_data.destination_data->destination_port;

    // Now pack the header.
    msg_header = pack_header(
        source_address, source_port, 
        destination_address, destination_port);

    // Now we send our message. network_send_pkt returns the number of
    // bytes it was able to send and -1 on failure.
    // XXX: check casting of msg_header
    bytes_sent = network_send_pkt(
        destination_address, sizeof(struct mini_header), (char *)msg_header, len, msg);  

    if (bytes_sent == -1) return 0;

    return bytes_sent - sizeof(struct mini_header);

}

/* Delivers a message to an unbound port's mailbox and makes it known that messages are 
 * available by calling semaphore_V on the port's available messages semaphore. This
 * function should be called by the network interrupt handler.
 */
void minimsg_dropoff_message(network_interrupt_arg_t *raw_msg)
{
    interrupt_level_t old_level;
    int local_unbound_port_number;
    miniport_t local_unbound_port;
    int get_result;

    // Check for NULL input.
    if (raw_msg == NULL) return;

    // Get the local unbound port number from the message header.
    local_unbound_port_number = get_destination_port(raw_msg->buffer);

    old_level = set_interrupt_level(DISABLED);

    // Get the miniport from the unbound ports table.
    local_unbound_port = (miniport_t) malloc(sizeof(miniport));
    get_result = hashtable_get(unbound_ports_table, local_unbound_port_number, (void **) &local_unbound_port);
    
    set_interrupt_level(old_level);

    if (get_result == -1) {
        return;
    }

    old_level = set_interrupt_level(DISABLED);

    // Dropoff the message by appending it to the port's message queue.
    queue_append(local_unbound_port->port_data.mailbox->received_messages, raw_msg);
 
    set_interrupt_level(old_level);

    // V on the semaphore to let threads know that messages are available
    semaphore_V(local_unbound_port->port_data.mailbox->available_messages_sema);

    
    return;
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
    network_interrupt_arg_t *raw_msg;
    int source_port;
    int payload_size;

    // Check for NULL input.
    if (local_unbound_port == NULL) return -1;

    // Check for available messages by calling a P on the mailbox's semaphore. If none are
    // available, this will cause the thread to block.
    semaphore_P(local_unbound_port->port_data.mailbox->available_messages_sema);

    // We have moved past the P, so a message is available. Grab the message by dequeueing it.
    raw_msg = (network_interrupt_arg_t *)malloc(sizeof(network_interrupt_arg_t));
    if (queue_dequeue(local_unbound_port->port_data.mailbox->received_messages, (void **) &raw_msg) == -1) 
    {
        return 0;  // if here, an error occurred and no bytes were received
    }
    
    // Now strip off and parse the header.
    source_port = get_source_port(raw_msg->buffer);
    *new_local_bound_port = miniport_create_bound(raw_msg->sender, source_port);
    strcpy(msg, get_payload(raw_msg->buffer)); 
    payload_size = raw_msg->size - sizeof(struct mini_header); //XXX: need to check this
    *len = payload_size;

    // We don't need the raw message anymore, so we free it.
    free(raw_msg);

    return payload_size;
}

