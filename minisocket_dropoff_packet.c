
/*
* The interrupt handling logic for any incoming packet.
*/
void minisocket_dropoff_packet(network_interrupt_arg_t *raw_packet){
    interrupt_level_t old_level;
    int *port_number_ptr;
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
    // If the packet was not from the connected socket and it's not destined to an open server, simply send a MSG_FIN.
    if(destination_socket->state != OPEN_SERVER &&
    	(!network_compare_network_addresses(source_socket_channel.address, destination_socket->destination_channel.address)
    	|| source_socket_channel.port_number != destination_socket->destination_channel.port_number))
    {    	
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
    		set_interrupt_level(old_level); 

    		minisocket_utils_send_packet_no_wait(destination_socket, MSG_ACK);   		
    		return;
    	}

    	destination_socket->state = CONNECTION_CLOSING;
    	port_number_ptr = (int *) malloc(sizeof(int));
    	*port_number_ptr = destination_socket->listening_channel.port_number;
    	destination_socket->mark_for_death_alarm = register_alarm(MS_TO_WAIT_TILL_CLOSE, 
                                                                  minisocket_utils_close_socket_handler, 
                                                                  port_number_ptr);
    	
    	if (destination_socket->state == SENDING) semaphore_V(destination_socket->ack_sema);
    	else semaphore_V(destination_socket->mailbox->available_messages_sema);

    	set_interrupt_level(old_level);
    	return;
    }

    if (destination_socket->state == CONNECTION_CLOSING) {
    	set_interrupt_level(old_level);
    	return; // not positive about this
    }

    if (msg_type == MSG_SYN && destination_socket->state == OPEN_SERVER 
    		&& !destination_socket->ack_received) {

    	destination_socket->ack_received = 1;
    	//V the waiting socket only if the alarm hasn't fired.
    	if(!destination_socket->ack_timedout){
    		semaphore_V(destination_socket->ack_sema);
    	}

    	//Set destination channel for the newly created connection.
    	network_address_copy(source_socket_channel.address, destination_socket->destination_channel.address);
    	destination_socket->destination_channel.port_number = source_socket_channel.port_number;

    	set_interrupt_level(old_level);
    	return;
    }

    if (msg_type == MSG_SYNACK 
    		&& (destination_socket->state == HANDSHAKING || destination_socket->state == OPEN_CONNECTION)
    		&& destination_socket->seq_number == ack_number && !destination_socket->ack_received) {

	    if (destination_socket->state == HANDSHAKING) {

	    	destination_socket->ack_received = 1;
	    	//V the waiting socket only if the alarm hasn't fired.
	    	if(!destination_socket->ack_timedout){
	    		semaphore_V(destination_socket->ack_sema);
	    	}
	    }
	    
    	set_interrupt_level(old_level);

    	// Send an ACK back.
    	destination_socket->ack_number = seq_number;
	    minisocket_utils_send_packet_no_wait(destination_socket, MSG_ACK);

    	return;
    }

    if (msg_type == MSG_ACK && destination_socket->state != HANDSHAKING 
    		&& destination_socket->seq_number == ack_number && !destination_socket->ack_received) {

    	destination_socket->ack_received = 1;
    	//V the waiting socket only if the alarm hasn't fired.
    	if(!destination_socket->ack_timedout){
    		semaphore_V(destination_socket->ack_sema);
    	}
    }

    if (raw_packet->size <= sizeof(struct mini_header_reliable)) {
    	set_interrupt_level(old_level);
    	return;
    }

    // Dropoff the message by appending it to the port's message queue, if not seen before by this socket.
    if(seq_number > destination_socket->ack_number){
        destination_socket->ack_number = seq_number;
        queue_append(destination_socket->mailbox->received_messages, raw_packet);
        // V on the semaphore to let threads know that messages are available
        semaphore_V(destination_socket->mailbox->available_messages_sema);
    }
 
    set_interrupt_level(old_level);

    // If the packet received was a data packet, send an ACK to let the
    // sender know that the packet was received (an ACK does not indicate
    // that the packet has been picked up by the socket, merely delivery).
    // We switch destination and source because we are sending from the
    // new packet's intended destination.
    minisocket_utils_send_packet_no_wait(destination_socket, MSG_ACK);
    
    return;
}