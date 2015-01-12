/* 
 * 
 * Filename: proxy.c
 * Author: Nipun Arora
 * Created: Mon Dec 29 17:25:28 2014 (-0500)
 * URL: http://www.nipunarora.net 
 * 
 * Description: 
 *
 */

#include <arpa/inet.h>
#include <errno.h>
#include <libgen.h>
#include <netdb.h>
#include <resolv.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wait.h>

#define BUF_SIZE 1024

#define READ  0
#define WRITE 1

#define SERVER_SOCKET_ERROR -1
#define SERVER_SETSOCKOPT_ERROR -2
#define SERVER_BIND_ERROR -3
#define SERVER_LISTEN_ERROR -4
#define CLIENT_SOCKET_ERROR -5
#define CLIENT_RESOLVE_ERROR -6
#define CLIENT_CONNECT_ERROR -7
#define CREATE_PIPE_ERROR -8
#define BROKEN_PIPE_ERROR -9

typedef enum {TRUE = 1, FALSE = 0} bool;

int create_socket(int port);
void sigchld_handler(int signal);
void sigterm_handler(int signal);
void server_loop();
void handle_client(int client_sock, struct sockaddr_in client_addr);
void forward_data_send(int source_sock, int destination_sock);
void forward_data_recv(int source_sock, int destination_sock);
int parse_options(int argc, char *argv[]);

int server_sock, client_sock, remote_sock, remote_port;
char *remote_host, *cmd_in, *cmd_out;
bool opt_in = FALSE, opt_out = FALSE;

/* Program start */
int main(int argc, char *argv[]) {
  int c, local_port;
  pid_t pid;

  local_port = parse_options(argc, argv);

  if (local_port < 0) {
    printf("Syntax: %s -l local_port -h remote_host -p remote_port [-i \"input parser\"] [-o \"output parser\"]\n", argv[0]);
    return 0;
  }

  if ((server_sock = create_socket(local_port)) < 0) { // start server
    perror("Cannot run server");
    return server_sock;
  }

  signal(SIGCHLD, sigchld_handler); // prevent ended children from
  signal(SIGTERM, sigterm_handler); // handle KILL signal

  switch(pid = fork()) {
  case 0:
    server_loop(); // daemonized child
    break;
  case -1:
    perror("Cannot daemonize");
    return pid;
  default:
    close(server_sock);
  }

  return 0;
}

/* Parse command line options */
int parse_options(int argc, char *argv[]) {
  bool l, h, p;
  int c, local_port;

  l = h = p = FALSE;

  while ((c = getopt(argc, argv, "l:h:p:i:o:")) != -1) {
    switch(c) {
    case 'l':
      local_port = atoi(optarg);
      l = TRUE;
      break;
    case 'h':
      remote_host = optarg;
      h = TRUE;
      break;
    case 'p':
      remote_port = atoi(optarg);
      p = TRUE;
      break;
    case 'i':
      opt_in = TRUE;
      cmd_in = optarg;
      break;
    case 'o':
      opt_out = TRUE;
      cmd_out = optarg;
      break;
    }
  }

  if (l && h && p) {
    return local_port;
  } else {
    return -1;
  }
}

/* Create server socket */
int create_socket(int port) {
  int server_sock, optval;
  struct sockaddr_in server_addr;

  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    return SERVER_SOCKET_ERROR;
  }

  if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    return SERVER_SETSOCKOPT_ERROR;
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
    return SERVER_BIND_ERROR;
  }

  if (listen(server_sock, 20) < 0) {
    return SERVER_LISTEN_ERROR;
  }

  return server_sock;
}

/* Handle finished child process */
void sigchld_handler(int signal) {
  while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* Handle term signal */
void sigterm_handler(int signal) {
  close(client_sock);
  close(server_sock);
  exit(0);
}

/* Main server loop */
void server_loop() {
  struct sockaddr_in client_addr;
  int addrlen = sizeof(client_addr);

  while (TRUE) {
    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addrlen);
    if (fork() == 0) { // handle client connection in a separate
                       // process
      close(server_sock);
      handle_client(client_sock, client_addr);
      exit(0);
    }
    close(client_sock);
  }

}

/* Handle client connection */
void handle_client(int client_sock, struct sockaddr_in client_addr){
  if ((remote_sock = create_connection()) < 0) {
    perror("Cannot connect to host");
    return;
  }

  if (fork() == 0) { // a process forwarding data from client to
    forward_data_send(client_sock, remote_sock);
    exit(0);
  }

  if (fork() == 0) { // a process forwarding data from remote socket
    forward_data_recv(remote_sock, client_sock);
    exit(0);
  }

  close(remote_sock);
  close(client_sock);
}

/* Forward data between sockets */
void forward_data_send(int source_sock, int destination_sock) {
  char buffer[BUF_SIZE];
  int n;

  while ((n = recv(source_sock, buffer, BUF_SIZE, 0)) > 0) { // read
    printf("Read data SEND %d bytes", n);
    send(destination_sock, buffer, n, 0); // send data to output
  }

  shutdown(destination_sock, SHUT_RDWR); // stop other processes from
  close(destination_sock);

  shutdown(source_sock, SHUT_RDWR); // stop other processes from using
  close(source_sock);
}

/* Forward data between sockets */
void forward_data_recv(int source_sock, int destination_sock) {
  char buffer[BUF_SIZE];
  int n;

  while ((n = recv(source_sock, buffer, BUF_SIZE, 0)) > 0) { // read
    printf("Read data  RECV %d bytes", n);
    send(destination_sock, buffer, n, 0); // send data to output
  }

  shutdown(destination_sock, SHUT_RDWR); // stop other processes from
  close(destination_sock);

  shutdown(source_sock, SHUT_RDWR); // stop other processes from using
  close(source_sock);
}


/* Create client connection */
int create_connection() {
  struct sockaddr_in server_addr;
  struct hostent *server;
  int sock;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    return CLIENT_SOCKET_ERROR;
  }

  if ((server = gethostbyname(remote_host)) == NULL) {
    errno = EFAULT;
    return CLIENT_RESOLVE_ERROR;
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  server_addr.sin_port = htons(remote_port);

  if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
    return CLIENT_CONNECT_ERROR;
  }

  return sock;
}

