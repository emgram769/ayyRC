#ifndef _IRC_H
#define _IRC_H

#include <stddef.h> /* size_t */

#define MAX_IRC_MESSAGE_LEN	512

/* A connection handler for each server the user is connected to. */
struct irc_connection_s;
#define MAX_IRC_ARGS	10
/* An irc msg. */
struct irc_msg_s {
	char* message;

	char* prefix;
	char* nickname;
	char* username;
	char* hostname;
	char* servername;

	char* command;
	char* args[MAX_IRC_ARGS];
	size_t args_len;
};

struct irc_connection_s* irc_connect(char* server, char* port);
void irc_set_recv_callback(struct irc_connection_s* conn,
													 void (*recv_callback)(struct irc_msg_s*));
struct irc_msg_s* irc_parse(char* s);
void irc_send_raw(struct irc_connection_s* conn, char* msg, size_t msg_len);

/* Utilities. */
void irc_set_username(struct irc_connection_s* conn,
											char* user, char* hostname,
											char* servername, char* realname);
void irc_set_password(struct irc_connection_s* conn, char* password);
void irc_set_nickname(struct irc_connection_s* conn, char* nickname);
void irc_send_privmsg(struct irc_connection_s* conn, char* name, char* msg);
void irc_join_channel(struct irc_connection_s* conn, char* channel);

/* Clean up. */
void irc_reap_connection(struct irc_connection_s* conn);
void irc_destroy_connection(struct irc_connection_s* conn);

#endif /* _IRC_H */
