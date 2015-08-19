#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "irc.h"

void print_results(struct irc_msg_s* msg) {
	if (!strcmp(msg->command, "PRIVMSG")) {
		printf("<%s> %s\n", msg->nickname, msg->message);
	} else {
		printf("$[%s] %s\n", msg->command, msg->message);
	}
}

int main(int argc, char *argv[]) { 
	char nickname[MAX_IRC_MESSAGE_LEN];
	printf("please enter a nick: ");
	fgets(nickname, MAX_IRC_MESSAGE_LEN, stdin);
	nickname[strlen(nickname)-1] = '\0';
	printf("your nick is %s, connecting.\n", nickname);

	struct irc_connection_s* conn = irc_connect("irc.rizon.sexy", "6667");
	irc_set_recv_callback(conn, &print_results);

	irc_set_username(conn, nickname, "localhost", "servername", nickname);
	irc_set_nickname(conn, nickname);
	sleep(2);
	irc_join_channel(conn, "#ayyRC");

	char buf[MAX_IRC_MESSAGE_LEN];
	while (1) {
		memset(buf, 0, MAX_IRC_MESSAGE_LEN);
		gets(buf);
		irc_send_privmsg(conn, "#ayyRC", buf);
	}

  irc_reap_connection(conn);
	return 0;
}

