/*
 *	Implementation of minisockets.
 */
#include <stdlib.h>

#include "minisocket.h"
#include "miniheader.h"
#include "synch.h"
#include "alarm.h"
#include "interrupts.h"
#include "queue.h"

//Port number conventions.
#define MAX_CLIENT_PORT_NUMBER (2<<15)-1
#define MIN_CLIENT_PORT_NUMBER (2<<14)
#define MAX_SERVER_PORT_NUMBER (2<<14)-1

// Initial timeout to wait in milliseconds.
#define INITIAL_TIMEOUT_MS 100
#define MAX_NUM_TIMEOUTS 7

// The delay in milliseconds between receiving a
// FIN and closing a socket (15 seconds). 
#define MS_TO_WAIT_TILL_CLOSE 15000

typedef enum {SERVER, CLIENT} socket_t;

typedef enum {
	OPEN_SERVER,
	HANDSHAKING, 
	OPEN_CONNECTION,
	SENDING,
	CONNECTION_CLOSING,
	CONNECTION_CLOSED
} state_t;

typedef struct socket_channel_t {
	int port_number;
	network_address_t address;
} socket_channel_t;

typedef struct mailbox {
	semaphore_t available_messages_sema;
	queue_t received_messages;
} *mailbox_t;

typedef struct minisocket
{
	// Listening ports are unbound ports, sending ports are bound ports
	socket_t type; // might be unecessary
	state_t state;
	socket_channel_t listening_channel;
	socket_channel_t destination_channel;
	int seq_number;
	int ack_number;
	semaphore_t ack_sema;
	int ack_received;
	mailbox_t mailbox;
} minisocket;

static int current_client_port_index;

minisocket_t current_sockets[MAX_CLIENT_PORT_NUMBER + 1];


 //Include util functions.
#include "minisocket_utils.c"


/* Initializes the minisocket layer. */
void minisocket_initialize()
{
	int i;
	current_client_port_index = MIN_CLIENT_PORT_NUMBER;
	i = 0;

	for(; i < MAX_CLIENT_PORT_NUMBER + 1; i++){
		current_sockets[i] = NULL;
	}
}


/* 
 * Listen for a connection from somebody else. When communication link is
 * created return a minisocket_t through which the communication can be made
 * from now on.
 *
 * The argument "port" is the port number on the local machine to which the
 * client will connect.
 *
 * Return value: the minisocket_t created, otherwise NULL with the errorcode
 * stored in the "error" variable.
 */
minisocket_t minisocket_server_create(int port, minisocket_error *error)
{
	minisocket_t 		new_server_socket; 
	socket_channel_t	new_listening_channel;
	semaphore_t 		new_available_messages_sema;
	semaphore_t 		new_ack_sema;
	network_address_t 	server_address;
	queue_t 			new_received_messages_q;
	mailbox_t			new_mailbox;

	// Check for null input.
	if (error == NULL) {
		return NULL;
	}

	// Check to see if the port number is invalid or in use.
	if (port < 0 || port > 32767) {
		*error = SOCKET_INVALIDPARAMS;
		return NULL;
	}
	if (current_sockets[port] != NULL) {
		*error = SOCKET_PORTINUSE;
		return NULL;
	} 

	// Create the server's listening channel.
	new_listening_channel.port_number = port;
	network_get_my_address(server_address);
	network_address_copy(server_address,new_listening_channel.address);
	
    // Create the ACK semaphore.
    new_ack_sema = semaphore_create();
    semaphore_initialize(new_ack_sema, 0);

    // Now set up the server's mailbox.
    new_available_messages_sema = semaphore_create();
    semaphore_initialize(new_available_messages_sema, 0);
    new_received_messages_q = queue_new();
    
    new_mailbox = (mailbox_t) malloc(sizeof(struct mailbox));
    new_mailbox->available_messages_sema = new_available_messages_sema;
    new_mailbox->received_messages = new_received_messages_q;

	// Initialize the server socket.
	new_server_socket = (minisocket_t) malloc(sizeof(minisocket));
	new_server_socket->type = SERVER;
	new_server_socket->state = OPEN_SERVER;
	new_server_socket->listening_channel = new_listening_channel;
	new_server_socket->ack_sema = new_ack_sema;
	new_server_socket->seq_number = 0;
	new_server_socket->ack_number = 0;
	new_server_socket->ack_received = 0;
	new_server_socket->mailbox = new_mailbox;

	// Now wait for a client to connect. This function does not return until
	// handshaking is complete and a connection is established. The server's
	// sending port will be initialized within this function.
	minisocket_utils_wait_for_client(new_server_socket, error);

	return new_server_socket;
}


/*
 * Initiate the communication with a remote site. When communication is
 * established create a minisocket through which the communication can be made
 * from now on.
 *
 * The first argument is the network address of the remote machine. 
 *
 * The argument "port" is the port number on the remote machine to which the
 * connection is made. The port number of the local machine is one of the free
 * port numbers.
 *
 * Return value: the minisocket_t created, otherwise NULL with the errorcode
 * stored in the "error" variable.
 */
minisocket_t minisocket_client_create(network_address_t addr, int port, minisocket_error *error)
{
	minisocket_t 		client_socket;
	socket_channel_t 	listening_channel;
	socket_channel_t 	destination_channel;
	semaphore_t 		ack_sema;
	semaphore_t 		available_messages_sema;
	queue_t 			msg_queue;
	mailbox_t			mailbox;

	network_address_t	netaddr;
	int 				valid_port;

	mini_header_reliable_t	syn_header;


	valid_port = minisocket_utils_client_get_valid_port();
	if(!valid_port){
		*error = SOCKET_NOMOREPORTS;
		return NULL;
	}

	if(port < 0 || port > MAX_SERVER_PORT_NUMBER){
		*error = SOCKET_INVALIDPARAMS;
		return NULL;
	}


	//Allocating client socket memory.
	network_get_my_address(netaddr);
	network_address_copy(netaddr, listening_channel.address);
	listening_channel.port_number = valid_port;

	network_address_copy(addr, destination_channel.address);
	destination_channel.port_number = port;

	ack_sema = semaphore_create();
	semaphore_initialize(ack_sema, 0);

    mailbox = (mailbox_t) malloc(sizeof(struct mailbox));

    available_messages_sema = semaphore_create();
    semaphore_initialize(available_messages_sema,0);
    mailbox->available_messages_sema = available_messages_sema;

    msg_queue = queue_new();
    mailbox->received_messages = msg_queue;	

    client_socket = (minisocket_t) malloc(sizeof(minisocket));

    client_socket->type = CLIENT;
    client_socket->state = HANDSHAKING;
    client_socket->listening_channel = listening_channel;
    client_socket->destination_channel = destination_channel;
    client_socket->seq_number = 1;
    client_socket->ack_number = 0;
    client_socket->ack_sema = ack_sema;
    client_socket->ack_received = 0;
    client_socket->mailbox = mailbox;

    current_sockets[valid_port] = client_socket;


    //Send SYN packet to begin connection to server.
    syn_header = minisocket_utils_pack_reliable_header(client_socket->listening_channel.address, client_socket->listening_channel.port_number,
    					 client_socket->destination_channel.address, client_socket->destination_channel.port_number,
    					 MSG_SYN, client_socket->seq_number, client_socket->ack_number);

    minisocket_utils_send_packet_and_wait(client_socket, sizeof(struct mini_header_reliable), (char *) syn_header, 0, NULL);

    *error = SOCKET_NOERROR;
    return client_socket;
}

/* 
 * Send a message to the other end of the socket.
 *
 * The send call should block until the remote host has ACKnowledged receipt of
 * the message.  This does not necessarily imply that the application has called
 * 'minisocket_receive', only that the packet is buffered pending a future
 * receive.
 *
 * It is expected that the order of calls to 'minisocket_send' implies the order
 * in which the concatenated messages will be received.
 *
 * 'minisocket_send' should block until the whole message is reliably
 * transmitted or an error/timeout occurs
 *
 * Arguments: the socket on which the communication is made (socket), the
 *            message to be transmitted (msg) and its length (len).
 * Return value: returns the number of successfully transmitted bytes. Sets the
 *               error code and returns -1 if an error is encountered.
 */
int minisocket_send(minisocket_t socket, minimsg_t msg, int len, minisocket_error *error)
{
	mini_header_reliable_t header;
	int payload_bytes_sent;
	int frag_size;
	int frag_bytes_sent;
	char *msg_buffer;

	// Check for NULL inputs.
	if (socket == NULL || msg == NULL || error == NULL) {
		if (error != NULL) *error = SOCKET_INVALIDPARAMS;
		return -1;
	}

	// Check if the connection is being closed.
	if (socket->state == CONNECTION_CLOSING) {
		*error = SOCKET_SENDERROR;
		return -1;
	}

	// Construct a header for the packet.
	header = minisocket_utils_pack_reliable_header(socket->listening_channel.address, socket->listening_channel.port_number,
								  socket->destination_channel.address, socket->destination_channel.port_number,
								  PROTOCOL_MINISTREAM, socket->seq_number, socket->ack_number);

	// Send the message. If it is too big, break it into fragments and send each one
	// individually.
	msg_buffer = (char *)msg;
	payload_bytes_sent = 0;
	while (payload_bytes_sent < len) {
		if (len - payload_bytes_sent > MAX_NETWORK_PKT_SIZE) {
			// We need to fragment our packet. We just send the first MAX_NETWORK_PKT_SIZE bytes in this iteration.
			frag_size = MAX_NETWORK_PKT_SIZE;
		} else {
			frag_size = len - payload_bytes_sent;
		}
		frag_bytes_sent = minisocket_utils_send_packet_and_wait(socket, sizeof(header), (char *) header, frag_size, &msg_buffer[payload_bytes_sent]);
		if (frag_bytes_sent == -1) {
			*error = SOCKET_SENDERROR;
			return payload_bytes_sent;
		}
		payload_bytes_sent += frag_bytes_sent;
	}

	*error = SOCKET_NOERROR;
	return payload_bytes_sent;
}

void minisocket_dropoff_packet(network_interrupt_arg_t *raw_packet)
{
    interrupt_level_t old_level;
    int port_number;
    int seq_number;
    int ack_number;
    char msg_type;
    socket_channel_t destination_socket_channel;
    socket_channel_t source_socket_channel;
    minisocket_t destination_socket;

    // Check for NULL input.
    if (raw_packet == NULL) return;

    // Get the local unbound port number from the message header.
    minisocket_utils_unpack_reliable_header(raw_packet->buffer, 
    					   &destination_socket_channel,
    					   &source_socket_channel,
    					   &msg_type,
    					   &seq_number,
    					   &ack_number);

    old_level = set_interrupt_level(DISABLED);

    // Find the socket the packet was intended for.
    port_number = destination_socket_channel.port_number;
    destination_socket = current_sockets[port_number];
    
    // If there is no destination socket at the port, drop the packet.
    if (destination_socket == NULL) {
    	set_interrupt_level(old_level);
        return;
    }
    // If the packet was not from the connected socket, simply send a MSG_FIN.
    if(!network_compare_network_addresses(source_socket_channel.address, destination_socket->destination_channel.address)
    	|| source_socket_channel.port_number != destination_socket->destination_channel.port_number){
    	
    	set_interrupt_level(old_level);

    	minisocket_utils_send_packet_no_wait(destination_socket, MSG_FIN);
    	return;
    }

    // Check to see if the message was a FIN.
    if (msg_type == MSG_FIN) {
	    // If the connection was closing, send an ACK back. Otherwise, set the state to closing.
	    // We register an alarm to close the connection in MS_TO_WAIT_TILL_CLOSE milliseconds and 
	    // call V on the appropriate semaphore to prevent blocking.
    	if (destination_socket->state == CONNECTION_CLOSING) {
    		minisocket_utils_send_packet_no_wait(destination_socket, MSG_ACK);
			set_interrupt_level(old_level);    		
    		return;
    	}

    	destination_socket->state = CONNECTION_CLOSING;
    	register_alarm(MS_TO_WAIT_TILL_CLOSE, minisocket_utils_close_socket, destination_socket);
    	
    	if (destination_socket->state == SENDING) semaphore_V(destination_socket->ack_sema);
    	else semaphore_V(destination_socket->mailbox->available_messages_sema);

    	set_interrupt_level(old_level);
    	return;
    }

    if (destination_socket->state == CONNECTION_CLOSING) {
    	set_interrupt_level(old_level);
    	return; // not positive about this
    }

    if (msg_type == MSG_SYNACK && destination_socket->state == HANDSHAKING
    		&& destination_socket->seq_number == ack_number && !destination_socket->ack_received) {
    	destination_socket->ack_received = 1;
    	semaphore_V(destination_socket->ack_sema);
    	set_interrupt_level(old_level);
    	return;
    }

    if (msg_type == MSG_ACK && destination_socket->state != HANDSHAKING 
    		&& destination_socket->seq_number == ack_number && !destination_socket->ack_received) {
    	destination_socket->ack_received = 1;
    	semaphore_V(destination_socket->ack_sema);
    }

    if (raw_packet->size <= sizeof(struct mini_header_reliable)) {
    	set_interrupt_level(old_level);
    	return;
    }

    // Dropoff the message by appending it to the port's message queue.
    queue_append(destination_socket->mailbox->received_messages, raw_packet);
 
    set_interrupt_level(old_level);

    // If the packet received was a data packet, send an ACK to let the
    // sender know that the packet was received (an ACK does not indicate
    // that the packet has been picked up by the socket, merely delivery).
    // We switch destination and source because we are sending from the
    // new packet's intended destination.
    minisocket_utils_send_packet_no_wait(destination_socket, MSG_ACK);

    // V on the semaphore to let threads know that messages are available
    semaphore_V(destination_socket->mailbox->available_messages_sema);
    
    return;
}

/*
 * Receive a message from the other end of the socket. Blocks until
 * some data is received (which can be smaller than max_len bytes).
 *
 * Arguments: the socket on which the communication is made (socket), the memory
 *            location where the received message is returned (msg) and its
 *            maximum length (max_len).
 * Return value: -1 in case of error and sets the error code, the number of
 *           bytes received otherwise
 */
int minisocket_receive(minisocket_t socket, minimsg_t msg, int max_len, minisocket_error *error)
{
	interrupt_level_t old_level;
	network_interrupt_arg_t *raw_msg;
	int bytes_received;
	int queue_empty;
	int dequeue_result;
	char *msg_buffer;
	int bytes_with_new_packet;

    // Check for NULL input.
    if (socket == NULL || msg == NULL || error == NULL) {
    	if (error != NULL) *error = SOCKET_INVALIDPARAMS;
    	return -1;
    }

    // Check if the socket is being closed.
    if (socket->state == CONNECTION_CLOSING) {
    	*error = SOCKET_RECEIVEERROR;
    	return -1;
    }

	// Check for available messages by calling a P on the mailbox's semaphore. If none are
	// available, this will cause the thread to block.
	semaphore_P(socket->mailbox->available_messages_sema);

	old_level = set_interrupt_level(DISABLED);

	queue_empty = 0;
	dequeue_result = queue_dequeue(socket->mailbox->received_messages, (void **) &raw_msg);
	if (dequeue_result == -1) {
		// This indicates that the connection is closing.
		*error = SOCKET_RECEIVEERROR;
		return -1;
	}
	if (raw_msg->size - sizeof(struct mini_header_reliable) > max_len) {
		queue_prepend(socket->mailbox->received_messages, raw_msg);
		semaphore_V(socket->mailbox->available_messages_sema);
		*error = SOCKET_NOERROR;
		return 0;
	}

	// Copy the payload of the packet into msg. We're done with the packet now, so free it.
	msg_buffer = (char *)msg;
	minisocket_utils_copy_payload(msg_buffer, raw_msg->buffer, raw_msg->size - sizeof(struct mini_header_reliable));
	bytes_received = raw_msg->size - sizeof(struct mini_header_reliable);
	free(raw_msg);
    
    // Pop messages off of the queue until the queue is empty or the total bytes received
    // is greater than max_len. We continue calling semaphore_P as the number of P's needs to
    // reflect the number of dequeues.
    while (!queue_empty && bytes_received < max_len) {
    	semaphore_P(socket->mailbox->available_messages_sema);
    	dequeue_result = queue_dequeue(socket->mailbox->received_messages, (void **) &raw_msg);
    	// Check if the queue was empty.
    	if (dequeue_result == -1) {
    		queue_empty = 1;
    		break;
 		}

 		// Check if this new packet will put us over max_len.
    	bytes_with_new_packet = bytes_received + raw_msg->size - sizeof(struct mini_header_reliable);
    	if (bytes_with_new_packet > max_len) {
    		// Put the message back on the queue and V the semaphore to reflect this.
    		queue_prepend(socket->mailbox->received_messages, raw_msg);
    		semaphore_V(socket->mailbox->available_messages_sema);
    		break;
    	}
	
		// Copy the payload of the packet into msg. We're done with the packet now, so free it.
    	minisocket_utils_copy_payload(&msg_buffer[bytes_received], raw_msg->buffer, raw_msg->size - sizeof(struct mini_header_reliable));
    	bytes_received += raw_msg->size - sizeof(struct mini_header_reliable);
		
		free(raw_msg);
    }

    set_interrupt_level(old_level);

    // Now we have filled msg as much as we can, so return the number of bytes we put into it.
    *error = SOCKET_NOERROR;
    return bytes_received;
}

/* Close a connection. If minisocket_close is issued, any send or receive should
 * fail.  As soon as the other side knows about the close, it should fail any
 * send or receive in progress. The minisocket is destroyed by minisocket_close
 * function.  The function should never fail.
 */
void minisocket_close(minisocket_t socket)
{
	mini_header_reliable_t fin_header;

	// Send a FIN packet to the connected socket to indicate that the connection
	// is closing.
	fin_header = minisocket_utils_pack_reliable_header(socket->listening_channel.address, socket->listening_channel.port_number,
    					 socket->destination_channel.address, socket->destination_channel.port_number,
    					 MSG_FIN, socket->seq_number, socket->ack_number);

    minisocket_utils_send_packet_and_wait(socket, sizeof(fin_header), (char *) fin_header, 0, NULL);

    // Whether we return successfully or not from send_packet_and_wait doesn't
    // matter, we are closing the connection either way.
    // We free all allocated memory.
	//free(socket->listening_channel);
	//free(socket->destination_channel); XXX: how do I free these?
	semaphore_destroy(socket->ack_sema);
	semaphore_destroy(socket->mailbox->available_messages_sema);
	queue_free(socket->mailbox->received_messages);

	free(socket);
}
