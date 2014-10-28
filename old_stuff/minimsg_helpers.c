/// A helper function to get an int from a char buffer.
int int_from_buffer(char *buffer) {
    return *(int *)buffer;
}

// A helper function to get a char buffer from an int.
    char* buffer_from_int(int n) {
    char *new_buffer = (char *)malloc(sizeof(int));
    new_buffer = (char *)&n;
    return new_buffer;
}

// A helper function to get a network_address_t from a char buffer.
// We have to return a pointer to a network_address_t because
// we cannot return a type defined as an array.
network_address_t* addr_from_buffer(char *buffer) {
    network_address_t *address_ptr = (network_address_t *)malloc(sizeof(network_address_t));
    network_address_copy((unsigned int *)buffer, *address_ptr);
    return address_ptr;
}

// A helper function to get a char buffer from a network_addr_t.
// XXX: Passing in address as a void pointer because of compile
// error. Fix this. (should be using network_address_t)
char* buffer_from_addr(void *address) {
    char *new_buffer = (char *)malloc(sizeof(network_address_t));
    network_address_copy((network_address_t) address, (network_address_t) new_buffer);
    new_buffer = (char *)&network_address_t;
    return new_buffer;
}
                                            
