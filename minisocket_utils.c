/*
 * A set of utility functions for minisockets.
 */

mini_header_reliable_t 
pack_reliable_header(network_address_t source_address, int source_port,
					 network_address_t destination_address, int destination_port,
					 char message_type, int seq_number, int ack_number)
{
	mini_header_reliable_t new_header = 
	(mini_header_reliable_t) malloc(sizeof(struct mini_header_reliable));

	new_header->protocol = PROTOCOL_MINISTREAM;
    pack_address(new_header->source_address, source_address);
    pack_unsigned_short(new_header->source_port, (short) source_port);
    pack_address(new_header->destination_address, destination_address);
    pack_unsigned_short(new_header->destination_port, (short) destination_port);
    new_header->message_type = message_type;
    pack_unsigned_int(new_header->seq_number, seq_number);
    pack_unsigned_int(new_header->ack_number, ack_number);

    return new_header;
}

void unpack_reliable_header(char *packet_buffer, socket_channel_t *destination_channel,
							socket_channel_t *source_channel, char *msg_type, int *seq_number, int *ack_number)
{
	// A temporary structure to make the implementation below more clear.
	mini_header_reliable_t header = (mini_header_reliable_t) packet_buffer;

	unpack_address(header->source_address, source_channel->address);
	source_channel->port_number = unpack_unsigned_short(header->source_port);
	unpack_address(header->destination_address, destination_channel->address);
	destination_channel->port_number = unpack_unsigned_short(header->destination_port);

	*msg_type = header->message_type;
	*seq_number = unpack_unsigned_int(header->seq_number);
	*ack_number = unpack_unsigned_int(header->ack_number);
}

/* Copies the payload into the memory location specified. It is assumed that the 
 * given memory location is valid and has enough room.
 */
void copy_payload(char *location_to_copy_to, char *buffer, int bytes_to_copy)
{
	char *payload = &buffer[sizeof(struct mini_header_reliable)];
	memcpy(location_to_copy_to, payload, bytes_to_copy);
	return;
}

/* Sets a socket's state to closed. Used as an alarm handler. */
void close_socket(void *socket_ptr) {
        minisocket_t socket = (minisocket_t) socket;
        socket->state = CONNECTION_CLOSED;
}
