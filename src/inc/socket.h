#ifndef _SOCKET_H
#define _SOCKET_H

#include <stddef.h> /* size_t */

int socket_send(int socket, char* buf, size_t buf_len);
int socket_recv(int socket, char* buf, size_t buf_len);
int socket_connect(char* server, char* port);

#endif /* _SOCKET_H */
