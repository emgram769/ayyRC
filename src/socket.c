#include "socket.h"

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int socket_send(int socket, char* buf, size_t buf_len) {
	int res = send(socket, buf, buf_len, 0);
	if (res < 0) {
		fprintf(stderr, "Send failed.\n");
	}
	return res;
}

int socket_recv(int socket, char* buf, size_t buf_len) {
	memset(buf, 0, buf_len);
  int res = recv(socket, buf, buf_len, 0);
	if (res == 0) {
		fprintf(stderr, "Connection closed.\n");
	}
	if (res < 0) {
		fprintf(stderr, "Recieve failed.\n");
	}
	return res;
}

int socket_connect(char* server, char* port) {
	struct addrinfo host, *host_list;

  memset(&host, 0, sizeof(host));
  host.ai_family = AF_UNSPEC;
  host.ai_socktype = SOCK_STREAM;
  host.ai_protocol = IPPROTO_TCP;

  int result = getaddrinfo(server, port, &host, &host_list);
  if (result != 0) {
    fprintf(stderr, "getaddrinfo failed! %s\n", gai_strerror(result));
  }

  int connected_socket = -1;
  connected_socket = socket(host_list->ai_family, host_list->ai_socktype, host_list->ai_protocol);

  if (connected_socket == -1) {
    fprintf(stderr, "Error opening the socket!\n");
    freeaddrinfo(host_list);
    return connected_socket;
  }

  do {
    result = connect(connected_socket, host_list->ai_addr, host_list->ai_addrlen);
		host_list = host_list->ai_next;
  } while (result == -1 && host_list->ai_next);

	if (result == -1) {
		close(connected_socket);
		connected_socket = -1;
		fprintf(stderr, "Unable to connect to server!\n");
		return connected_socket;
	}

  return connected_socket;
}


