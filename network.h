#ifndef __NETWORK_H__
#define __NETWORK_H__

/*
 * network.h:
 *      Low-level network interface.
 *
 *      This interface defines a low-level network interface for sending and
 *      receiving packets between pseudo-network interfaces located on the 
 *      same or different hosts.
 */

#define MAX_NETWORK_PKT_SIZE    8192

/* network_address_t's should be treated as opaque types. See functions below */
typedef unsigned int network_address_t[2];

/*******************************************************************************
*  Network interrupt handler                                                   *
*******************************************************************************/

/* the argument to the network interrupt handler */
typedef struct {
    network_address_t sender;
    char buffer[MAX_NETWORK_PKT_SIZE];
    int size;
} network_interrupt_arg_t;

/* the type of an interrupt handler.  These functions are responsible for freeing
 * the argument that is passed in */
typedef void (*network_handler_t)(network_interrupt_arg_t *arg);

/*
 * network_initialize should be called before clock interrupts start
 * happening (or with clock interrupts disabled).  The initialization
 * procedure returns 0 on success, -1 on failure.  The function
 * handler(data) is called when a network packet arrives.
 *
 * You must call this function before you call any other network
 * functions, including network_translate_hostname().
 */
int network_initialize(network_handler_t network_handler);

/*
 * only used for testing; normally, you should not have to call this
 * function. it should be called before network_initialize, and sets
 * the local UDP port to use for miniports, as well as the port number
 * to use for "remote" ports; by this mechanism, it's possible to run
 * a pair of processes on the same computer without their ports
 * conflicting. of course, this is a hack.
 */
void network_udp_ports(short myportnum, short otherportnum);


/******************************************************************************
*  Functions for sending packets                                               *
******************************************************************************/

/*
 * network_send_pkt returns the number of bytes sent if it was able to
 * successfully send the data.  Returns -1 otherwise.
 */
int
network_send_pkt(network_address_t dest_address,
                 int hdr_len, char * hdr,
                 int  data_len, char * data);


/*******************************************************************************
*  Functions for working with network addresses                                *
*******************************************************************************/

/*
 * network_my_address returns the network_address that can be used
 * to send a packet to the caller's address space.  Note that
 * an address space can send a packet to itself by specifying the result of
 * network_get_my_address() as the dest_address to network_send_pkt.
 */
void network_get_my_address(network_address_t my_address);

/* look up the given host and return the corresponding network address.
 * Returns TODO
 */
int network_translate_hostname(char* hostname, network_address_t address);

/*
 * Compares network addresses. Returns 0 if different and
 * nonzero if identical.
 */
int network_compare_network_addresses(network_address_t addr1,
                                      network_address_t addr2);

/*
 * write the network address in a human-readable way, into a buffer of length
 * "length"; will return -1 if the string is too short, else 0. the address
 * will be in the form "the.text.ip.address:port", e.g. "128.84.223.105:20".
 * Note: the port is an actual UDP port, not a miniport! 
 */
int network_format_address(network_address_t address, char* string, int length);

/* zero the address, so as to make it invalid */
void network_address_blankify(network_address_t addr);

/* copy address "original" to address "copy". */
void network_address_copy(network_address_t original, network_address_t copy);

#endif /*__NETWORK_H_*/

