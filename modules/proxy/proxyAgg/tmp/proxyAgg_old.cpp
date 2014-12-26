// 
// Filename: proxyAgg.cpp
// Author: Nipun Arora
// Created: Fri Nov 28 19:43:37 2014 (-0500)
// URL: http://www.nipunarora.net 
// 
// Description: 
// 


//C HEADER START

//#define _GNU_SOURCE

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
#include <pthread.h>
#include <proxyAgg.h>

#define NUM_THREADS 100

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

//typedef enum {TRUE = 1, FALSE = 0} bool;

//C HEADER END


#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <iostream>

#include <boost/atomic.hpp>

boost::atomic_int producer_count(0);
boost::atomic_int consumer_count(0);

boost::lockfree::queue<int> queue(128);

const int iterations = 10000000;
const int producer_thread_count = 4;
const int consumer_thread_count = 4;

//c variables start
 int buffer_size;
 int server_sock, client_sock, remote_sock, remote_port, duplicate_destination_sock, duplicate_port;
 char *remote_host, *duplicate_host, *cmd_in, *cmd_out;
 bool opt_in = false, opt_out = false, asynch = false, synch = false;
 bool l, h, p, x, d, a, s, b = false;
//c variables end
pthread_t threads[NUM_THREADS];
int thread_count=0;

/* Parse command line options */
int parse_options(int argc, char *argv[]) 
{
	
	int c, local_port;
	l = h = p = x = d = false;

	while ((c = getopt(argc, argv, "l:h:p:x:d:b:as")) != -1) 
	{
		switch(c) 
		{
			case 'l':
			local_port = atoi(optarg);
			l = true;
			DEBUG_PRINT("local_port =  %d ", local_port);
			break;

			case 'h':
			remote_host = optarg;
			h = true;
			DEBUG_PRINT("remote_host =  %s ", remote_host);
			break;
	
			case 'p':
			remote_port = atoi(optarg);
			p = true;
			DEBUG_PRINT("production_port =  %d ", remote_port);			
			break;
	
			case 'x':
			duplicate_host = optarg;
			x = true;
			DEBUG_PRINT("duplicate_host =  %s ", duplicate_host);
			break;
	
			case 'd':
			duplicate_port = atoi(optarg);
			d = true;
			DEBUG_PRINT("duplicate_port =  %d ", duplicate_port);
			break;

			case 'a':
			a = true;
			asynch = true;
			DEBUG_PRINT("asynchrounous mode ");
			break;

			case 's':
			s = true;
			synch = true;
			DEBUG_PRINT("synchrounous mode ");
			break;

			case 'b':
			b = true;
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

	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	  DEBUG_PRINT("socket counld not be created ");
	  return SERVER_SOCKET_ERROR;
	}

	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
	  DEBUG_PRINT("setsockopt error");
	  return SERVER_SETSOCKOPT_ERROR;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
	  DEBUG_PRINT("bind error");
	  return SERVER_BIND_ERROR;
	}

	if (listen(server_sock, 20) < 0) {
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
void sigterm_handler(int signal) 
{
	close(client_sock);
	close(server_sock);
	close(duplicate_destination_sock);
	exit(0);
}

void server_loop()
{
  struct sockaddr_in client_addr;
  int addrlen = sizeof(client_addr);
 
  while (TRUE) 
    {
      client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addrlen);    
      int res = pthread_create(&threads[thread_count], NULL, handle_client, (void *)t);

      if (fork() == 0) 
        { // handle client connection in a separate process
         
          close(server_sock);
          handle_client(client_sock, client_addr);
          exit(0);
        }	
      close(client_sock);
      
    }
}

int main(int argc, char* argv[])
{
  using namespace std;
  cout<<"Starting proxy Aggregator";

  int c, local_port;
  local_port = parse_options(argc, argv);
  
  if (local_port < 0) 
    {
      printf("Syntax: %s -l local_port -h remote_host -p remote_port -x duplicate_host -d duplicate_port -a|s (a= asynchrounous mode, s = synchrounous mode) -b buffer_size \n", argv[0]);
      return 0;
    }

  if((server_sock = create_socket(local_port)) < 0)
    {
      perror("Cannot run the server");
      return server_sock;
    }

}
