/*
 *	Implementation of minisockets.
 */
#include <stdlib.h>

#include "minisocket.h"
#include "synch.h"

// Initial timeout to wait in milliseconds.
#define INITIAL_TIMEOUT_MS 100
#define MAX_NUM_TIMEOUTS 7

typedef enum {SERVER, CLIENT} socket_t;

typedef enum {
	OPEN_SERVER,
	HANDSHAKING, 
	OPEN_CONNECTION,
	SENDING,
	CONNECTION_CLOSING 
} state_t;

typedef struct socket_port_t {
	int port_number;
	network_address_t address;
} socket_port_t;

typedef struct mailbox_t {
	int port_number;
	semaphore_t event_sema;
	queue_t received_messages;
} mailbox_t;

typedef struct minisocket
{
	// Listening ports are unbound ports, sending ports are bound ports
	socket_t socket_type; // might be unecessary
	state_t state;
	socket_port_t listening_port;
	socket_port_t destination_port;
	mailbox_t mailbox;
} minisocket;

int current_client_port_index;

minisocket_t *server_socket_array;
minisocket_t *client_socket_array;

/* Initializes the minisocket layer. */
void minisocket_initialize()
{
	current_client_port_index = 32768;
	server_socket_array = (minisocket_t *)malloc(32767 * sizeof(minisocket_t));
	client_socket_array = (minisocket_t *)malloc(32767 * sizeof(minisocket_t));
}

mini_header_reliable_t 
pack_reliable_header(network_address_t source_address, int source_port,
					 network_address_t destination_address, int destination_port,
					 char message_type, int seq_number, int ack_number)
{
	mini_header_reliable_t new_header = 
	(mini_header_reliable_t) malloc(sizeof(struct mini_header_reliable));

	new_header->protocol = PROTOCOL_MINIDATAGRAM;
    pack_address(new_header->source_address, source_address);
    pack_unsigned_short(new_header->source_port, (short) source_port);
    pack_address(new_header->destination_address, dest_address);
    pack_unsigned_short(new_header->destination_port, (short) dest_port);
    new_header->message_type = message_type;
    pack_unsigned_int(new_header->seq_number, seq_number);
    pack_unsigned_int(new_header->ack_number, ack_number);

    return new_header;
}

void unpack_reliable_header(char *packet_buffer, socket_port_t *destination_socket_port,
							socket_port_t *source_socket_port, int *seq_number, int *ack_number)
{
	// A temporary structure to make the implementation below more clear.
	mini_header_reliable_t header = (mini_header_reliable_t) packet_buffer;

	destination_socket_port = (socket_port_t *)malloc(sizeof(struct(socket_port_t)));
	source_socket_port = (socket_port_t *)malloc(sizeof(struct(socket_port_t)))

	unpack_address(header->source_address, source_socket_port->address);
	*source_socket_port->port_number = unpack_unsigned_short(header->source_port);
	unpack_address(header->destination_address, destination_socket_port->address)
	*destination_socket_port->port_number = unpack_unsigned_short(header->destination_port);
	*seq_number = unpack_unsigned_int(header->seq_number);
	*ack_number = unpack_unsigned_int(header->ack_number);
}

void semaphore_V_wrapper(void *semaphore_ptr) {
        semaphore_t semaphore = (semaphore_t) semaphore_ptr;
        semaphore_V(semaphore);
}

/* Sends a packet and waits INITIAL_TIMEOUT_MS milliseconds for an ACK. If no 
 * ACK is received within that time, the packet is resent up to MAX_NUM_TIMEOUTS
 * times. Upon each resending of the packet, the time to wait doubles.
 * Returns the number of bytes sent on success and -1 on error.
 */
int send_and_wait(minisocket_t sending_socket, int hdr_len, char* hdr,
				  int data_len, char* data)
{
	// TODO: implement fragmentation, timeouts.
	minisocket_error *error;
	mini_header_reliable_t header;
	int bytes_sent;
	alarm_id timeout_alarm;

	int timeout_to_wait = INITIAL_TIMEOUT_MS;
	int num_timeouts = 0;

	while (num_timeouts < MAX_NUM_TIMEOUTS) {
		// Send the packet.
		bytes_sent  = network_send_pkt(address, hdr_len, hdr, data_len, data);
		
		// Schedule the timeout alarm.
		timeout_alarm = register_alarm(timeout_to_wait,
									   semaphore_V_wrapper,
									   sending_socket->destination_port->event_sema);

		// Wait for an ACK. This function will return -1 if the alarm
		// goes off before an ACK is received.
		bytes_received = minisocket_receive(
			server, (minimsg_t) header, sizeof(struct mini_header_reliable), error);

		if (bytes_received != -1 && header->message_type == MSG_ACK) {
			// We received an ACK, so we can return success.
			return bytes_sent;
		}

		if (bytes_received == -1)

		timeout_to_wait = timeout_to_wait * 2;
		num_timeouts++;
	}

	return -1;
}

/* Used for sending control packets, when we don't need to wait for an ACK. */
void send_no_wait(char msg_type, socket_port_t source_socket, socket_port_t destination_socket)
{
	mini_header_reliable_t header;

	pack_reliable_header(source_socket->address, source_socket->port_number,
						 destination_socket->address, destination_socket->port_number,
						 msg_type, 0, 0);
	// XXX: what happens when &data = NULL in network_send_pkt?
	network_send_pkt(destination_socket->address, sizeof(header), header, 0, NULL);
}

/* Sets the server's state to OPEN_SERVER and waits for a SYN from a client.
 * When a SYN is received, the server goes to the HANDSHAKING state and
 * responds with a SYNACK. If an ACK is received from the client after
 * this, then the server's state is set to OPEN_CONNECTION and this function
 * returns. If no ACK is received, then the server goes back to waiting
 * for a SYN.
 */ 
void wait_for_client(minisocket_t server, minisocket_error *error) {
	int bytes_received;
	int bytes_sent;
	socket_port_t new_sending_port;
	// header is used for both receiving and sending control packets.
	mini_header_reliable_t header;

	server->state = OPEN_SERVER;

	while(1) {
		// First we wait for a SYN.
		while (!server->state == HANDSHAKING) {
			bytes_received = minisocket_receive(
				server,
				(minimsg_t) header,
				0,
				&error);

			if (bytes_received == -1) {
				// error will be set by minisocket_receive
				// TODO: return some indication of error.
				// What is a receive error?? What is proper behavior here?
				continue;
			}

			// Check to see if the message received was a SYN.
			if (header->message_type == MSG_SYN) {
				server->sending_port = new_sending_port;
				server->state = HANDSHAKING;
			}
		}

		// We received a SYN, so send a SYNACK back.
		header = pack_header(
			server->sending_port->address, server->sending_port->port_number,
			server->destination_port->address, server->destination_port->port_number,
			SYNACK, 0, 0);
		server->state = SENDING;
		bytes_sent = send_packet(server, SYNACK, NULL, &error);

		if (bytes_sent != sizeof(mini_header_reliable)) {
			// We did not receive an ACK to our SYNACK, so go back to
			// waiting for clients.
			server->sending_port = NULL;
			server->state = OPEN_SERVER;
			continue;
		} else {
			// We received an ACK, so a connection has been established.
			server->state = OPEN_CONNECTION;
			return;
		}

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
	minisocket_t new_server_socket; 
	socket_port_t new_listening_port;
	socket_port_t new_sending_port;
	semaphore_t new_event_sema;
	network_address_t server_address;

	// Check for null input.
	if (error == NULL) {
		return NULL;
	}

	// Check to see if the port number is invalid or in use.
	if (port < 0 || port > 32767) {
		*error = SOCKET_INVALIDPARAMS;
		return NULL;
	}
	if (server_socket_array[port] != NULL) {
		*error = PORTINUSE;
		return NULL;
	} 

	// Create the server's listening port.
	new_listening_port = (socket_t)malloc(sizeof(socket_t));
	new_listening_port->port_number = port;
	network_get_my_address(server_address);
	new_listening_port->address = server_address;

    // Now set up the server's mailbox.
    new_event_sema = semaphore_create();
    semaphore_initialize(new_event_sema, 0);
    new_received_messages_q = queue_new();
    
    new_mailbox = (mailbox *)malloc(sizeof(mailbox));
    new_mailbox->port_number = port_number;
    new_mailbox->event_sema = new_event_sema;
    new_mailbox->received_messages = new_received_messages_q;

	// Initialize the server socket.
	new_server = (minisocket_t) malloc(sizeof(minisocket));
	new_server_socket->socket_type = SERVER;
	new_server_socket->state = OPEN_SERVER;
	new_server_socket->listening_port = new_listening_port;
	new_server_socket->event_sema = new_event_sema;

	// Now wait for a client to connect. This function does not return until
	// handshaking is complete and a connection is established. The server's
	// sending port will be initialized within this function.
	wait_for_client(new_server);

	return new_server;
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

}

void minisocket_dropoff_packet(network_interrupt_arg_t *raw_packet)
{
    interrupt_level_t old_level;
    int port_number;
    socket_port_t destination_socket_port;
    socket_port_t source_socket_port;

    // Check for NULL input.
    if (raw_msg == NULL) return;

    // Get the local unbound port number from the message header.
    unpack_reliable_header(raw_packet->buffer, 
    					   &destination_socket_port,
    					   &source_socket_port,
    					   &seq_number,
    					   &ack_number);

    old_level = set_interrupt_level(DISABLED);

    // Find the socket the packet was intended for.
    if (destination_socket_port->port_number > 32767) {
    	destination_socket = client_socket_array[port_number - 32767];
    } else {
    	destination_socket = server_socket_array[port_number];
    }
    
    // If there is no destination socket at the port, drop the packet.
    if (destination_socket == NULL) {
    	set_interrupt_level(old_level);
        return;
    }

    // Dropoff the message by appending it to the port's message queue.
    queue_append(destination_socket->mailbox->received_messages, raw_msg);
 
    set_interrupt_level(old_level);

    // If the packet received was a data packet, send an ACK to let the
    // sender know that the packet was received (an ACK does not indicate
    // that the packet has been picked up by the socket, merely delivery).
    // We switch destination and source because we are sending from the
    // new packet's intended destination.
    if (raw_msg->size > sizeof(mini_header_reliable)) {
    	send_control_packet(MSG_ACK, destination_socket_port, source_socket_port);
    }

    // V on the semaphore to let threads know that messages are available
    semaphore_V(destination_socket_port->port_data.mailbox->event_sema);
    
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
	mini_header_reliable header;
	state_t state;

	state = socket->state;
	if (state == HANDSHAKING || state == SERVER_OPEN || state == SENDING) {
		// return the header in msg
		return sizeof(mini_header_reliable);
	}

	// pull ALL packets off of the queue

}

/* Close a connection. If minisocket_close is issued, any send or receive should
 * fail.  As soon as the other side knows about the close, it should fail any
 * send or receive in progress. The minisocket is destroyed by minisocket_close
 * function.  The function should never fail.
 */
void minisocket_close(minisocket_t socket)
{

}
