/* 
 * 
 * Filename: proxy.c
 * Author: Nipun Arora
 * Created: Wed Sep 24 14:19:13 2014 (-0400)
 * URL: http://www.nipunarora.net 
 * 
 * Description: 
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
 void forward_data(int source_sock, int destination_sock);
 //void forward_data_ext(int source_sock, int destination_sock, char *cmd);
 void forward_data_synch(int source_sock, int destination_sock, int duplicate_destination_sock);
 void receive_data_synch(int source_sock, int destination_sock, int duplicate_destination_sock);
 int parse_options(int argc, char *argv[]);

 int server_sock, client_sock, remote_sock, remote_port, duplicate_destination_sock, duplicate_port;
 char *remote_host, *duplicate_host, *cmd_in, *cmd_out;
 bool opt_in = FALSE, opt_out = FALSE, asynch = FALSE, synch = FALSE;
 bool l, h, p, x, d, a, s = FALSE;

/* Program start */
 int main(int argc, char *argv[]) 
{
 	int c, local_port;
 	pid_t pid;

 	local_port = parse_options(argc, argv);

 	if (local_port < 0) 
 	{
 		printf("Syntax: %s -l local_port -h remote_host -p remote_port -x duplicate_host -d duplicate_port [-i \"input parser\"] [-o \"output parser\"]\n", argv[0]);
 		return 0;
 	}

  /*
    Here we start the proxy server with it listening at the local port specified
   */
    if ((server_sock = create_socket(local_port)) < 0) 
    { // start server
    	perror("Cannot run server");
    	return server_sock;
    }

  signal(SIGCHLD, sigchld_handler); // prevent ended children from becoming zombies
  signal(SIGTERM, sigterm_handler); // handle KILL signal

  /*
    Here we fork of a daemon which starts the server_loop function
   */
    switch(pid = fork()) 
    {
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
int parse_options(int argc, char *argv[]) 
{
	int c, local_port;
	l = h = p = x = d = FALSE;

	while ((c = getopt(argc, argv, "l:h:p:x:d:i:o:as")) != -1) 
	{
		switch(c) 
		{
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
	
			case 'x':
			duplicate_host = optarg;
			x = TRUE;
			break;
	
			case 'd':
			duplicate_port = atoi(optarg);
			d = TRUE;
			break;
	
			case 'i':
			opt_in = TRUE;
			cmd_in = optarg;
			break;

			case 'o':
			opt_out = TRUE;
			cmd_out = optarg;
			break;

			case 'a':
			a = TRUE;
			asynch = TRUE;
			break;

			case 's':
			s = TRUE;
			synch = TRUE;
			break;
		}
	}

	//if asynch or synch are not specified only local port, remote host, and remote port are required
	// if either asynch or synch are specified both the duplicate port, and duplicate host must also be specified
	if (l && h && p && !a && !s) {
		return local_port;
	} 
	if (l && h && p && (a||s) && x && d){
		return local_port;
	}
	else {
		return -1;
	}
}

/* 
Create server socket this is socket at which the proxy is binded and listens for incoming connections
*/
int create_socket(int port) {
	int server_sock, optval;
	struct sockaddr_in server_addr;

	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		return SERVER_SOCKET_ERROR;
	}

	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) 
	{
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
void sigterm_handler(int signal) 
{
	close(client_sock);
	close(server_sock);
	exit(0);
}

/* Main server loop */
void server_loop() 
{
	struct sockaddr_in client_addr;
	int addrlen = sizeof(client_addr);

	while (TRUE) 
	{
		client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addrlen);    	
    	if (fork() == 0) 
    	{ // handle client connection in a separate process
    		close(server_sock);
    		handle_client(client_sock, client_addr);
    		exit(0);
    	}	
    	close(client_sock);
	}
}

/* Handle client connection */
void handle_client(int client_sock, struct sockaddr_in client_addr)
{
	//create connection to main production server
	if ((remote_sock = create_connection()) < 0) {
		perror("Cannot connect to host");
		return;
	}
	
	//if duplication is chosen either synch or asynch must be specified
	if(synch||asynch){
		if((duplicate_destination_sock = create_dup_connection()) < 0){
			perror("Could not connec to to duplicate host");
			return;
		}	
	}

/**
Create a fork to forward data from client to remote host
*/		
  if (fork() == 0) { // a process forwarding data from client to remote socket
  	if (opt_out) 
  	{
		//forward_data_ext(client_sock, remote_sock, cmd_out);
  	} 
  	else 
  	{
  		if(!synch && !asynch)
	  		forward_data(client_sock, remote_sock);
	  	if(synch){
	  		forward_data_synch(client_sock, remote_sock, duplicate_destination_sock);
	  	}
  	}
  	exit(0);
  }
/**
Create a fork to forward data from remote host to client
*/
  if (fork() == 0) 
  { // a process forwarding data from remote socket to client
  	if (opt_in) 
  	{
  		//forward_data_ext(remote_sock, client_sock, cmd_in);
  	} 
  	else 
  	{
		if(!synch && !asynch)
			forward_data(remote_sock, client_sock);
		if(synch)
			receive_data_synch(remote_sock,client_sock,duplicate_destination_sock);
  	}
  	exit(0);
  }

  close(remote_sock);
  close(client_sock);
}

/* Forward data between sockets */
void forward_data(int source_sock, int destination_sock) {
	char buffer[BUF_SIZE];
	int n;

  //put in error condition for -1, currently the socket is shutdown
  while ((n = recv(source_sock, buffer, BUF_SIZE, 0)) > 0) 
  	{ // read data from input socket
    	send(destination_sock, buffer, n, 0); // send data to output socket
	}

  shutdown(destination_sock, SHUT_RDWR); // stop other processes from using socket
  close(destination_sock);

  shutdown(source_sock, SHUT_RDWR); // stop other processes from using socket
  close(source_sock);
}

/* Forward data to destination and duplicate destination node synchronously*/
void receive_data_synch(int source_sock, int destination_sock, int duplicate_destination_sock)
{
	char buffer[BUF_SIZE];
	int n;

	char dup_buffer[BUF_SIZE];
	int dup_n;

  	while ((n = recv(source_sock, buffer, BUF_SIZE, 0)) > 0) // read data from input socket
	{ 
    	send(destination_sock, buffer, n, 0); // send data to output socket
    	if(recv(duplicate_destination_sock, dup_buffer, n, 0) !=n) //data is read from buffer and discarded
    	{
    		printf("Error in receiving data from duplicate");
    	}
  	}

  	/*
  	Close connections on all ends
  	1. Closing connection on destination sockets
  	*/
  	shutdown(destination_sock, SHUT_RDWR); // stop other processes from using socket
  	close(destination_sock);

  	//2. Close connection on duplicate socket
  	shutdown(duplicate_destination_sock, SHUT_RDWR);
  	close(duplicate_destination_sock);
  
  	//3. Close connection from source socket
  	shutdown(source_sock, SHUT_RDWR); // stop other processes from using socket
  	close(source_sock);
}

/* Forward data to destination and duplicate destination node synchronously*/
void forward_data_synch(int source_sock, int destination_sock, int duplicate_destination_sock)
{
	char buffer[BUF_SIZE];
	int n;

  	while ((n = recv(source_sock, buffer, BUF_SIZE, 0)) > 0) // read data from input socket
  	{ 
    	send(destination_sock, buffer, n, 0); // send data to output socket
      	send(duplicate_destination_sock, buffer, n, 0);// duplicate data to duplicate socket
  	}

  	/*
  	Close connections on all ends
  	1. Closing connection on destination sockets
  	*/
  	shutdown(destination_sock, SHUT_RDWR); // stop other processes from using socket
  	close(destination_sock);

  	//2. Close connection on duplicate socket
  	shutdown(duplicate_destination_sock, SHUT_RDWR);
  	close(duplicate_destination_sock);
  
  	//3. Close connection from source socket
  	shutdown(source_sock, SHUT_RDWR); // stop other processes from using socket
  	close(source_sock);
}

/* Create connection to duplicate host*/
int create_dup_connection() {
	struct sockaddr_in server_addr;
	struct hostent *server;
	int sock;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return CLIENT_SOCKET_ERROR;
	}
	//usage duplicate host
	if ((server = gethostbyname(duplicate_host)) == NULL) {
		errno = EFAULT;
		return CLIENT_RESOLVE_ERROR;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	//usage duplicate_port
	server_addr.sin_port = htons(duplicate_port);

	if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		return CLIENT_CONNECT_ERROR;
	}

	return sock;
}

/* Create client connection */
int create_connection() {
	struct sockaddr_in server_addr;
	struct hostent *server;
	int sock;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return CLIENT_SOCKET_ERROR;
	}
	//usage production host
	if ((server = gethostbyname(remote_host)) == NULL) {
		errno = EFAULT;
		return CLIENT_RESOLVE_ERROR;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	
	//usage production port
	server_addr.sin_port = htons(remote_port);

	if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		return CLIENT_CONNECT_ERROR;
	}

	return sock;
}
