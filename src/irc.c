#include "irc.h"
#include "socket.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct irc_connection_s {
  int socket;
  void (*recv_callback)(struct irc_msg_s*);
  pthread_t recv_thread;
  int keep_alive;
};

struct irc_msg_s* irc_parse(char* s) {
  struct irc_msg_s* msg = malloc(sizeof(struct irc_msg_s*));
  size_t idx = 0;

  /* Get the prefix. */
  if (s[idx] == ':') {
    msg->prefix = &(s[idx+1]);
    while (s[++idx] != ' ') {
      /* TODO: Parse out the rest of the prefix. */
      if (s[idx] == '!') {
        msg->nickname = msg->prefix;
        s[idx] = '\0';
      }
    }
    /* We got to the first space. */
    s[idx++] = '\0';
  }

  /* Parse out the command. */
  msg->command = &s[idx];
  while (s[++idx] != ' '); /* Get the next space. */
  s[idx++] = '\0';

  /* Parse out the command arguments, if any. */
  msg->args_len = 0;
  while (s[idx] != ':' && s[idx] != '\0') {
    msg->args[msg->args_len++] = &s[idx];
    while (s[++idx] != ' ' && s[idx] != '\0'); /* Get the next space. */
    s[idx++] = '\0';
  }

  /* The rest is the "message." */
  msg->message = &s[++idx];
  return msg;
}

/* For storing the current message. */
char irc_msg_buf[MAX_IRC_MESSAGE_LEN];
/* For getting the next message. */
char irc_buf[MAX_IRC_MESSAGE_LEN];
size_t irc_buf_offset;
struct irc_msg_s* irc_get_msg(struct irc_connection_s* conn) {
  /* Check if there is a valid IRC message in the irc_buffer. */
  size_t idx = 0;
   /* Eat up non-delimiters. */
  while (irc_buf[idx] != '\0' &&
         ++idx < MAX_IRC_MESSAGE_LEN &&
         irc_buf[idx] != '\r');

  if (irc_buf[idx] == '\r' && irc_buf[idx + 1] == '\n') {
    irc_buf[idx] = '\0';
    irc_buf[idx + 1] = '\0';
    size_t msg_buf_len = idx + 2;
    memset(irc_msg_buf, 0, MAX_IRC_MESSAGE_LEN);
    memcpy(irc_msg_buf, irc_buf, msg_buf_len);
    struct irc_msg_s* msg = irc_parse(irc_msg_buf);
    memmove(irc_buf, irc_buf + msg_buf_len, MAX_IRC_MESSAGE_LEN - msg_buf_len);
    memset(irc_buf + MAX_IRC_MESSAGE_LEN - msg_buf_len, 0, msg_buf_len);
    return msg;
  }

  if (idx == MAX_IRC_MESSAGE_LEN) {
    fprintf(stderr, "Ah fuck, this shouldn't happen.\n");
  }

  /* Fill up the buffer and try again. */
  if (irc_buf[idx] == '\0') {
    socket_recv(conn->socket, &irc_buf[idx], MAX_IRC_MESSAGE_LEN - idx);
  }
  return irc_get_msg(conn);
}

int irc_handle_bullshit(struct irc_connection_s* conn, struct irc_msg_s* msg) {
  if (!strcmp(msg->command, "PING")) {
    char buf[MAX_IRC_MESSAGE_LEN];
    sprintf(buf, "PONG :%s\r\n", msg->message);
    irc_send_raw(conn, buf, strlen(buf));
    return 1;
  }
  if (!strcmp(msg->message, "VERSION")) {
    char buf[MAX_IRC_MESSAGE_LEN];
    printf("wooooo!@\n");
    sprintf(buf, "PRIVMSG %s :VERSION %s\r\n", msg->nickname, "ayyRC 0.0.1");
    irc_send_raw(conn, buf, strlen(buf));
    return 1;
  }
  return 0;
}

void irc_set_recv_callback(struct irc_connection_s* conn, void (*recv_callback)(struct irc_msg_s*)) {
  conn->recv_callback = recv_callback;
}

void *irc_connection_thread(void* args) {
  struct irc_connection_s* conn = (struct irc_connection_s*)args;
  /* We spin. */
  while (conn->keep_alive) {
    /* Allocate msg. */
    struct irc_msg_s* msg = irc_get_msg(conn);

    if (!irc_handle_bullshit(conn, msg)) {
      if (conn->recv_callback) {
        (*conn->recv_callback)(msg);
      }
    }

    /* Free msg. */
    free(msg);
  }
  return NULL;
}

struct irc_connection_s* irc_connect(char* server, char* port) {
  struct irc_connection_s* connection = malloc(sizeof(struct irc_connection_s));
  /* Connect to the server. */
  connection->socket = socket_connect(server, port);
  /* Keep the connection alive. */
  connection->keep_alive = 1;
  /* Clear callback. */
  connection->recv_callback = NULL;

  /* Initialize a thread to listen for data. */
  if (pthread_create(&connection->recv_thread, NULL, *irc_connection_thread, connection)) {
    fprintf(stderr, "Could not spawn recv thread.\n");
  }
  return connection;
}

void irc_reap_connection(struct irc_connection_s* conn) {
  /* Let the connection die. */
  conn->keep_alive = 0;
  pthread_join(conn->recv_thread, NULL);
  irc_destroy_connection(conn);
}

void irc_set_username(struct irc_connection_s* conn,
                      char* user,
                      char* hostname,
                      char* servername,
                      char* realname) {
  char buf[MAX_IRC_MESSAGE_LEN];
  sprintf(buf, "USER %s %s %s :%s\r\n", user, hostname, servername, realname);
  irc_send_raw(conn, buf, strlen(buf));
}

void irc_set_password(struct irc_connection_s* conn, char* password) {
  char buf[MAX_IRC_MESSAGE_LEN];
  sprintf(buf, "PASS %s\r\n", password);
  irc_send_raw(conn, buf, strlen(buf));
}

void irc_set_nickname(struct irc_connection_s* conn, char* name) {
  char buf[MAX_IRC_MESSAGE_LEN];
  sprintf(buf, "NICK %s\r\n", name);
  irc_send_raw(conn, buf, strlen(buf));
}

void irc_join_channel(struct irc_connection_s* conn, char* name) {
  char buf[MAX_IRC_MESSAGE_LEN];
  sprintf(buf, "JOIN %s\r\n", name);
  irc_send_raw(conn, buf, strlen(buf));
}

void irc_send_privmsg(struct irc_connection_s* conn, char* name, char* msg) {
  char buf[MAX_IRC_MESSAGE_LEN];
  sprintf(buf, "PRIVMSG %s :%s\r\n", name, msg);
  irc_send_raw(conn, buf, strlen(buf));
}

void irc_destroy_connection(struct irc_connection_s* conn) {
  free(conn);
  return;
}

void irc_send_raw(struct irc_connection_s* conn, char* msg, size_t msg_len) {
  socket_send(conn->socket, msg, msg_len);
}

