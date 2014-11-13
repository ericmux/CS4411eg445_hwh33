
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
    socket_channel_t destination_channel;
    socket_channel_t source_channel;
    minisocket_t destination_socket;
    int addresses_equal;
    int ports_equal;

    // Check for NULL input.
    if (raw_packet == NULL) return;

    // Get the local unbound port number from the message header.
    minisocket_utils_unpack_reliable_header(raw_packet->buffer, 
    					   &destination_channel,
    					   &source_channel,
    					   &msg_type,
    					   &seq_number,
    					   &ack_number);

    old_level = set_interrupt_level(DISABLED);

    // Find the socket the packet was intended for.
    port_number = destination_channel.port_number;
    destination_socket = current_sockets[port_number];
    
    // If there is no destination socket at the port or the destination socket has been
    // closed, drop the packet.
    if (destination_socket == NULL || destination_socket->state == CONNECTION_CLOSED) {
        set_interrupt_level(old_level);
        return;
    }

    addresses_equal = network_compare_network_addresses(
        source_channel.address, destination_socket->destination_channel.address);
    ports_equal = source_channel.port_number == destination_socket->destination_channel.port_number;
    // If the packet was not from the connected socket and it's not destined to an open server, 
    // simply send a MSG_FIN.
    if(destination_socket->state != OPEN_SERVER && (!addresses_equal || !ports_equal))
    {       
        set_interrupt_level(old_level);

        minisocket_utils_send_packet_no_wait(destination_socket, MSG_FIN);
        return;
    }

    if (msg_type == MSG_SYN) {
        // If the destination socket is an open server who has not received a SYN yet, V the
        // semaphore and set the ack received bit.
        if (destination_socket->state == OPEN_SERVER && !destination_socket->ack_received) {
            destination_socket->ack_received = 1;
            semaphore_V(destination_socket->ack_sema);

            //Set destination channel for the newly created connection.
            network_address_copy(source_channel.address, destination_socket->destination_channel.address);
            destination_socket->destination_channel.port_number = source_channel.port_number;
        }
        set_interrupt_level(old_level);
        return;
    }

    if (msg_type == MSG_SYNACK) {

	    if (destination_socket->state == HANDSHAKING && destination_socket->type == CLIENT
            && !destination_socket->ack_received) {

	    	destination_socket->ack_received = 1;
    		semaphore_V(destination_socket->ack_sema);

        	// Send an ACK back.
        	destination_socket->ack_number = seq_number;
    	    minisocket_utils_send_packet_no_wait(destination_socket, MSG_ACK);
        }
        
        set_interrupt_level(old_level);
        return;
    }

    if (msg_type == MSG_ACK) {
        if (destination_socket->state == OPEN_CONNECTION || destination_socket->state == SENDING) {
            
            // If the packet indicates that the receiver has ancknowleged our most recent packet and 
            // we haven't already, V on the ACK semaphore.
            if (ack_number == destination_socket->seq_number && !destination_socket->ack_received) {
                destination_socket->ack_received = 1;
                destination_socket->ack_number = seq_number;
                semaphore_V(destination_socket->ack_sema);
            }

            // If this is the next data packet, append it to the mailbox and V on the mailbox semaphore.
            // Then send an ACK back to let the sender know the packet was received.
            if (seq_number == destination_socket->ack_number + 1
                && raw_packet->size > sizeof(struct mini_header_reliable)) {

                queue_append(destination_socket->mailbox->received_messages, raw_packet);
                semaphore_V(destination_socket->mailbox->available_messages_sema);
                
                destination_socket->ack_number = seq_number;
                minisocket_utils_send_packet_no_wait(destination_socket, MSG_ACK);
            }
        }
    }

    if (msg_type == MSG_FIN) {
        // If the connection is already closing, just send an ACK back. 
        if (destination_socket->state == CONNECTION_CLOSING) {
            destination_socket->ack_number = seq_number;
            minisocket_utils_send_packet_no_wait(destination_socket, MSG_ACK);          

            set_interrupt_level(old_level); 
            return;
        }

        // Otherwise, set the state to closing. We register an alarm to close the 
        // connection in MS_TO_WAIT_TILL_CLOSE milliseconds and call V on the 
        // appropriate semaphore to prevent blocking.
        destination_socket->state = CONNECTION_CLOSING;
        port_number_ptr = (int *) malloc(sizeof(int));
        *port_number_ptr = destination_socket->listening_channel.port_number;
        destination_socket->mark_for_death_alarm = register_alarm(MS_TO_WAIT_TILL_CLOSE, 
                                                                  minisocket_utils_close_socket_handler, 
                                                                  port_number_ptr);
        
        if (destination_socket->state == OPEN_CONNECTION) {
            semaphore_V(destination_socket->mailbox->available_messages_sema);
        }
        else {
            semaphore_V(destination_socket->ack_sema);
        }

        set_interrupt_level(old_level);
        return;
    }

    // This code shouldn't be reachable, but just in case.
    set_interrupt_level(old_level);    
    return;
}