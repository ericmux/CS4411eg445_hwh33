
#include <stdlib.h>
#include <stdio.h>

/* header definition for unreliable packets */
typedef struct mini_header
{
    char protocol;

    char source_address[8];
    char source_port[2];

    char destination_address[8];
    char destination_port[2];

} *mini_header_t;

// Takes in a packet as a char buffer and returns the source port.
int get_source_port(char *packet_buffer) {
// The source port is located after the protocol (a char)
// and the source address (an 8-byte char buffer).
    int *int_ptr = (int *) &packet_buffer[sizeof(char) + 8 * sizeof(char)];
    return *int_ptr; 
}

/*
// Takes in a packet as a char buffer and returns the packet's payload.
minimsg_t get_payload(char *packet_buffer) {
    // The payload is located after the entire header, which
    // is a struct of type mini_header.
    return (minimsg_t) packet_buffer[sizeof(struct mini_header)];
}
*/

int main() {

    int i;
    char buffer[21];

    buffer[0] = 'c';
    for (i = 1; i < 9; i++) {buffer[i] = 'c';}
    int *source_port = (int *) &buffer[9];
    *source_port = 42;
    printf("%d\n", *source_port);
    for (i = 13; i < 21; i++) {buffer[i] = '&';}

    int returned_port = get_source_port(buffer);

    printf("Source port: %d; Port returned: %d\n", *source_port, returned_port); 

}
