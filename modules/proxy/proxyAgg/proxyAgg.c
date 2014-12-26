/* 
 * 
 * Filename: proxyAgg.c
 * Author: Nipun Arora
 * Created: Thu Dec 25 20:21:59 2014 (-0500)
 * URL: http://www.nipunarora.net 
 * 
 * Description: 
 */


#define _GNU_SOURCE

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
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
#include <fcntl.h>

#define DEBUG TRUE

#ifdef DEBUG
#define DEBUG_PRINT(fmt, args...)    fprintf(stderr, fmt, ## args)
#else
#define DEBUG_PRINT(fmt, args...)    /* Don't do anything in release builds */
#endif

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
 void forward_data_asynch(int source_sock, int destination_sock);
 void forward_data_synch(int source_sock, int destination_sock, int duplicate_destination_sock);
 void receive_data_synch(int source_sock, int destination_sock, int duplicate_destination_sock);
 void receive_data_asynch(int source_sock);
 void forward_data_pipe(int destination_sock);
 int parse_options(int argc, char *argv[]);
 void setBufferSize(int fd, int buffer);
 void getPipeSize(int pipe_in, int pipe_out);
 void getPipeSize2(int pipe_in, int pipe_out); 
 void print_timeofday();
 int pfds[2];
 int buffer_size;
 int server_sock, client_sock, remote_sock, remote_port, duplicate_destination_sock, duplicate_port;
 char *remote_host, *duplicate_host, *cmd_in, *cmd_out;
 bool opt_in = FALSE, opt_out = FALSE, asynch = FALSE, synch = FALSE;
 bool l, h, p, x, d, a, s, b = FALSE;

FILE *log_file, *stats_file;

/* Program start */
int main(int argc, char *argv[]) {
  int c, local_port;
  pid_t pid;
  
  local_port = parse_options(argc, argv);

  if (local_port < 0) 
    {
      printf("Syntax: %s -l local_port -h remote_host -p remote_port -x duplicate_host -d duplicate_port -a|s (a= asynchrounous mode, s = synchrounous mode) -b buffer_size \n", argv[0]);
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

  //    fclose(log_file);
  //    fclose(stats_file);
    
  return 0;
}

/* Parse command line options */
int parse_options(int argc, char *argv[]) {
	
  int c, local_port;
  l = h = p = x = d = FALSE;

  while ((c = getopt(argc, argv, "l:h:p:x:d:b:as")) != -1) 
    {
      switch(c) 
        {
        case 'l':
          local_port = atoi(optarg);
          l = TRUE;
          DEBUG_PRINT("local_port =  %d ", local_port);
          break;

        case 'h':
          remote_host = optarg;
          h = TRUE;
          DEBUG_PRINT("remote_host =  %s ", remote_host);
          break;
	
        case 'p':
          remote_port = atoi(optarg);
          p = TRUE;
          DEBUG_PRINT("production_port =  %d ", remote_port);			
          break;
	
        case 'x':
          duplicate_host = optarg;
          x = TRUE;
          DEBUG_PRINT("duplicate_host =  %s ", duplicate_host);
          break;
	
        case 'd':
          duplicate_port = atoi(optarg);
          d = TRUE;
          DEBUG_PRINT("duplicate_port =  %d ", duplicate_port);
          break;

        case 'a':
          a = TRUE;
          asynch = TRUE;
          DEBUG_PRINT("asynchrounous mode ");
          break;

        case 's':
          s = TRUE;
          synch = TRUE;
          DEBUG_PRINT("synchrounous mode ");
          break;

        case 'b':
          b = TRUE;
          buffer_size = atoi(optarg);
          DEBUG_PRINT("buffer_size = %d ", buffer_size);
          break;

        }
    }

  DEBUG_PRINT("\n");

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
      DEBUG_PRINT("socket counld not be created ");
      return SERVER_SOCKET_ERROR;
    }

  if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) 
    {
      DEBUG_PRINT("setsockopt error");
      return SERVER_SETSOCKOPT_ERROR;
    }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) 
    {
      DEBUG_PRINT("bind error");
      return SERVER_BIND_ERROR;
    }

  if (listen(server_sock, 20) < 0) 
    {
      DEBUG_PRINT("listen error");
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
  close(duplicate_destination_sock);
  exit(0);
}

/* Main server loop */
void server_loop() {
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

/*
  get peername info
*/
void getpeerinfo(int s)
{
  struct sockaddr_storage addr;
  socklen_t len;
  len = sizeof(addr);
  char ipstr[INET6_ADDRSTRLEN];
  int connection_counter=0;
  int port;

  getpeername(s, (struct sockaddr*)&addr, &len);
  // deal with both IPv4 and IPv6:
  if (addr.ss_family == AF_INET) 
    {
      struct sockaddr_in *s = (struct sockaddr_in *)&addr;
      port = ntohs(s->sin_port);
      inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
    } 
  else 
    { // AF_INET6
      struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
      port = ntohs(s->sin6_port);
      inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
    }

  printf("Peer IP address: %s\n", ipstr);
  printf("Peer port      : %d\n", port);

}


void handle_client(int client_sock, struct sockaddr_in client_addr)
{
  getpeerinfo(client_sock);
}
