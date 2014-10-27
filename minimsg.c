// /*
//  *  Implementation of minimsgs and miniports.
//  */
// #include <stdlib.h>

// #include "minimsg.h"
// #include "hashtable.h"

// // XXX: is changing this to a typedef okay?
// typedef struct miniport
// {
//     int unbound_port_number;
//     int bound_port_number;  // -1 indicates no port
//     network_address_t destination_address; //XXX: this isn't right..
// }miniport; 

// // The port numbers assigned to new ports.
// int current_bound_port_number;

// // If a port number is in unbound_port_table, then
// // it is currently being used.
// hashtable_t bound_port_table;

// /* performs any required initialization of the minimsg layer.
//  */
// void
// minimsg_initialize()
// {
//     current_bound_port_number = 32768;
//     bound_port_table = hashtable_create();
// }

// /* Creates an unbound port for listening. Multiple requests to create the same
//  * unbound port should return the same miniport reference. It is the responsibility
//  * of the programmer to make sure he does not destroy unbound miniports while they
//  * are still in use by other threads -- this would result in undefined behavior.
//  * Unbound ports must range from 0 to 32767. If the programmer specifies a port number
//  * outside this range, it is considered an error.
//  */
// miniport_t
// miniport_create_unbound(int port_number)
// {
//     miniport_t new_miniport = (miniport_t) malloc(sizeof(miniport));

//     if (port_number < 0 || port_number > 32767) return NULL;

//     new_miniport->unbound_port_number = port_number;
//     // We are just setting this port up for listening, so the following are NULL.
//     new_miniport->bound_port_number = -1;
//     //new_miniport->destination_address = NULL;

//     return new_miniport;
// }

// /* Creates a bound port for use in sending packets. The two parameters, addr and
//  * remote_unbound_port_number together specify the remote's listening endpoint.
//  * This function should assign bound port numbers incrementally between the range
//  * 32768 to 65535. Port numbers should not be reused even if they have been destroyed,
//  * unless an overflow occurs (ie. going over the 65535 limit) in which case you should
//  * wrap around to 32768 again, incrementally assigning port numbers that are not
//  * currently in use.
//  */
// miniport_t
// miniport_create_bound(network_address_t addr, int remote_unbound_port_number)
// {
//     miniport_t new_miniport;

//     // Obtain the port number for the bound port.
//     // TODO: what to do when there are no available ports?
//     while (hashtable_key_exists(bound_port_table, current_bound_port_number)) {
//         current_bound_port_number++;
//     }

//     new_miniport = (miniport_t)malloc(sizeof(miniport));

//     // Set up our destination address and port number.
//     new_miniport->unbound_port_number = remote_unbound_port_number;
//     //new_miniport->destination_address = addr;
//     // Set the bound port to use for sending.
//     new_miniport->bound_port_number = current_bound_port_number;

//     // The last thing we do is increment the bound port counter.
//     current_bound_port_number++;
//     if (current_bound_port_number > 65535) current_bound_port_number = 32768;

//     return 0;
// }

// /* Destroys a miniport and frees up its resources. If the miniport was in use at
//  * the time it was destroyed, subsequent behavior is undefined.
//  */
// void
// miniport_destroy(miniport_t miniport)
// {
// }

// /* Sends a message through a locally bound port (the bound port already has an associated
//  * receiver address so it is sufficient to just supply the bound port number). In order
//  * for the remote system to correctly create a bound port for replies back to the sending
//  * system, it needs to know the sender's listening port (specified by local_unbound_port).
//  * The msg parameter is a pointer to a data payload that the user wishes to send and does not
//  * include a network header; your implementation of minimsg_send must construct the header
//  * before calling network_send_pkt(). The return value of this function is the number of
//  * data payload bytes sent not inclusive of the header.
//  */
// int
// minimsg_send(miniport_t local_unbound_port, miniport_t local_bound_port, minimsg_t msg, int len)
// {
//     return 0;
// }

// /* Receives a message through a locally unbound port. Threads that call this function are
//  * blocked until a message arrives. Upon arrival of each message, the function must create
//  * a new bound port that targets the sender's address and listening port, so that use of
//  * this created bound port results in replying directly back to the sender. It is the
//  * responsibility of this function to strip off and parse the header before returning the
//  * data payload and data length via the respective msg and len parameter. The return value
//  * of this function is the number of data payload bytes received not inclusive of the header.
//  */
// int minimsg_receive(miniport_t local_unbound_port, miniport_t* new_local_bound_port, minimsg_t msg, int *len)
// {
//     return 0;
// }

