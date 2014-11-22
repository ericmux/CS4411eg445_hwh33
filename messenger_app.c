/*
 * A simple messenger app. Each messenger can only communicate
 * with one other messenger at any given time.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "minisocket.h"
#include "network.h"
#include "synch.h"
#include "minithread.h"

// This isn't actually the maximum message size; there is none. It just
// defines the number of bytes each segment of the message contains.
// XXX: what should this number be??
#define MSG_SIZE 100

// Unique to each user.
int user_id;

// From Stack Overflow, trims trailing newline characters. Found here:
// http://stackoverflow.com/questions/25615916/removing-newline-from-fgets
void trim(char *str) {
	int i = strlen(str)-1;
  	if ((i > 0) && (str[i] == '\n'))
    	str[i] = '\0';
}

char* get_input(int input_size) {
	char *user_input = (char *)malloc(input_size);
	fgets(user_input, input_size+1, stdin);
	trim(user_input);
	return user_input;
}

int send_messages(int* socket_ptr) {
	char *user_input;
	minisocket_error *error;
	minisocket_t socket;

	socket = (minisocket_t) socket_ptr;
	while (1) {
		user_input = get_input(MSG_SIZE);
		if (user_input[0] == '.' && user_input[1] == '\0') {
			// need to kill receive thread
			return 0;
		}
		//printf("%s: %s\n", username, user_input);

		minisocket_send(socket, user_input, strlen(user_input), error);
	}

	return 0;
}

void receive_messages(minisocket_t socket) {
	minimsg_t msg;
	minisocket_error *error;

	while (1) {
		minisocket_receive(socket, msg, MINIMSG_MAX_MSG_SIZE, error);

		printf("%s\n", msg);
	}
}

void start_session(minisocket_t socket) {
	minithread_fork(send_messages, socket);
	receive_messages(socket);
}

/* Wait for a messenger by initializing this messenger as a server
 * and waiting for another messenger to connect as a client.
 */
int wait_for_partner() {
	int port;
	minisocket_t server_socket;
	minisocket_error *error;

	printf("Please specify a port to use:\n");
	port = atoi(get_input(5));

	printf("Waiting for partner...\n");
	minisocket_server_create(port, error);

	if (error != NULL) return 0;

	printf("Connected!\n");

	start_session(server_socket);

}

/* Start a chat by connecting to a waiting messenger. This messenger
 * is the client.
 */
int start_chat() {
	minisocket_t client_socket;
	minisocket_error *error;
	network_address_t target_address;
	int target_port;

	printf("Please give the target address:\n");
	//get_input
	printf("Please give the target port:\n");
	target_port = atoi(get_input(5));

	printf("Connecting to target.\n");
	client_socket = minisocket_client_create(target_address, target_port, error);

	if (error != NULL) return 0;

	printf("Connected!\n");

	start_session(client_socket);
}


int main() {
	char *username;
	char *user_input;
	int decision_made;

	printf("Enter your username (10 characters or less):\n");
	username = get_input(10);

	printf("If you would like to initiate a chat, please enter 'chat'.\n");
	printf("If you would like to wait for a chat partner, please enter 'wait'\n");
	decision_made = 0;
	while (!decision_made) {
		user_input = get_input(4);
		if (strncmp(user_input, "chat", 4) == 0) {
			decision_made = 1;
			start_chat();
		} else if(strncmp(user_input, "wait", 4) == 0) {
			decision_made = 1;
			wait_for_partner();
		}
	}

}
