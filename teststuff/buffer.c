
#include <stdlib.h>
#include <stdio.h>

// char buffer -> unsigned int
unsigned int int_from_buffer(char *buffer) {
    return *(unsigned int *) buffer; 
}

// unsigned int -> char buffer
char* buffer_from_int(unsigned int n) {
    char *new_buffer = (char *)malloc(sizeof(unsigned int));
    new_buffer = (char *)&n;
    return new_buffer;
}

int main() {
    unsigned int x = 8675;

    char *c = buffer_from_int(x);

    unsigned int x_prime = int_from_buffer(c);

    printf("I am x: %d and I am x': %d\n", x, x_prime);    

    printf("Size of int: %d; Size of unsigned int: %d\n", sizeof(int), sizeof(unsigned int));

    int y = 65535;

    printf("Does this work? Should be 65535: %d\n", y);

    return 0;
}
