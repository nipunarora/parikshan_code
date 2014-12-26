/* 
 * 
 * Filename: proxy.c
 * Author: Nipun Arora
 * Created: Wed Sep 24 14:19:13 2014 (-0400)
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
 int main(int argc, char *argv[]) 
{
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
int parse_options(int argc, char *argv[]) 
{
	
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
int create_socket(int port) 
{
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
void sigchld_handler(int signal) 
{
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

/* Main server loop */
void server_loop() 
{
  struct sockaddr_in client_addr;
  int addrlen = sizeof(client_addr);

  while (TRUE) 
    {
      client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addrlen);    	
      if (fork() == 0) { // handle client connection in a separate process
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
  //fprintf(stats_file, " Created connection\n");
  DEBUG_PRINT(" Created connection\n");
  print_timeofday();
  if ((remote_sock = create_connection()) < 0) 
    {
      perror("Cannot connect to host");
      return;
    }
  
  //if duplication is chosen either synch or asynch must be specified
  if(synch||asynch)
    {
      DEBUG_PRINT(" Created duplicate connection\n");
      printf("format string" ,a0,a1);
      int_timeofday();
      //fprintf(stats_file, " Created duplicate connection\n");
      
      if((duplicate_destination_sock = create_dup_connection_synch()) < 0)
        {
          perror("Could not connect to to duplicate host");
          return;
        }	
    }

  if(asynch)
    {
      if(pipe(pfds)<0)
        {
          perror("pipe ");
          exit(1);
        }
      setNonblocking(pfds[1]);
      DEBUG_PRINT("pipe size %d \n",fcntl(pfds[1],F_GETPIPE_SZ));    
      if(b)
        {
          setBufferSize(pfds[1],buffer_size);
          //setBufferSize(pfds[0],buffer_size);
        }      
    }

  /**
     T1: Create a fork to forward data from client to remote host
  */		

  if (fork() == 0) 
    { 
      if(!synch && !asynch)
        //write to remote socket
        forward_data(client_sock, remote_sock);
      if(synch)
        {
          //write to remote socket and duplicate socket
          forward_data_synch(client_sock, remote_sock, duplicate_destination_sock);
        }
      if(asynch)
        {
          //write to remote socket and pipe
          forward_data_asynch(client_sock,remote_sock);
  	}
      
      exit(0);
    }

  /**
     T2: Create a fork to forward data from remote host to client
  */
  if (fork() == 0) 
    { 
      if(!synch)
        forward_data(remote_sock, client_sock);
      if(synch)
        receive_data_synch(remote_sock,client_sock,duplicate_destination_sock);
    
      exit(0);
    }

  if(asynch)
    {
      
      /**
         T3: Create a fork to forward data asynchrounously to duplicate
      */	  
      if(fork() == 0)
        {        
          forward_data_pipe(duplicate_destination_sock);
          exit(0);		
        }
      /**
         T4: Create a fork to read data asynchrounously to duplicate
      */
      if(fork() == 0)
        {
          receive_data_asynch(duplicate_destination_sock); 	
          exit(0);
        }
    }

  /*    
        if(fork() == 0){
        while(1)
        {
	sleep(1);
	getPipeSize(pfds[0], pfds[1]);
        //getPipeSize2(pfds[0], pfds[1]);
        }
        }
  */

  /*
    Close all socket opened in this handle
  */
  close(remote_sock);
  close(client_sock);
  close(duplicate_destination_sock);

}

/* Create connection to duplicate host*/
int create_dup_connection_synch() {
	
  struct sockaddr_in server_addr;
  struct hostent *server;
  int sock;

  
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("Client socket error");
    return CLIENT_SOCKET_ERROR;
  }
  //usage duplicate host
  //printf("Host %s \n", duplicate_host);

  if ((server = gethostbyname(duplicate_host)) == NULL) {
    errno = EFAULT;
    printf("Client resolve error");
    return CLIENT_RESOLVE_ERROR;
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  //usage duplicate_port
  server_addr.sin_port = htons(duplicate_port);
	
  //printf("Port ID %d \n", duplicate_port);

  if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
    printf("Client connect error");
    return CLIENT_CONNECT_ERROR;
  }

  return sock;
}

/* Create client connection */
int create_connection() {

  DEBUG_PRINT("creating connection... \n");

  struct sockaddr_in server_addr;
  struct hostent *server;
  int sock;

  if ((sock = socket(AF_INET,SOCK_STREAM, 0)) < 0) {
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

/* Forward data between sockets */
void forward_data(int source_sock, int destination_sock) {
  char buffer[BUF_SIZE];
  int n;

  //put in error condition for -1, currently the socket is shutdown
  while ((n = recv(source_sock, buffer, BUF_SIZE, 0)) > 0)// read data from input socket 
    { 
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

/* Forward data between sockets */
void forward_data_asynch(int source_sock, int destination_sock) 
{

  char buffer[BUF_SIZE];
  int n;

  //put in error condition for -1, currently the socket is shutdown
  while ((n = recv(source_sock, buffer, BUF_SIZE, 0)) > 0)// read data from input socket 
    { 
      send(destination_sock, buffer, n, 0); // send data to output socket
      if( write(pfds[1],buffer,n) < 0 )//send data to pipe
        {
          //fprintf(stats_file,"buffer_overflow \n");
          print_timeofday();
          perror("Buffer overflow? ");
        }
      //DEBUG_PRINT("Data sent to pipe %s \n", buffer);
    }

  shutdown(destination_sock, SHUT_RDWR); // stop other processes from using socket
  close(destination_sock);

  shutdown(source_sock, SHUT_RDWR); // stop other processes from using socket
  close(source_sock);
}

/* Forward data from pipe to duplicate */
void forward_data_pipe(int destination_sock) 
{

  char buffer[BUF_SIZE];
  int n;

  //put in error condition for -1, currently the socket is shutdown
  while ((n = read(pfds[0], buffer, BUF_SIZE)) > 0)// read data from pipe socket 
    { 
      //sleep(1);
      //DEBUG_PRINT("Data received in pipe %s \n", buffer);
      send(destination_sock, buffer, n, 0); // send data to output socket
    }

  shutdown(destination_sock, SHUT_RDWR); // stop other processes from using socket
  close(destination_sock);
}

void receive_data_asynch(int source_sock){
  char buffer[BUF_SIZE];
  int n;

  while ((n = recv(source_sock, buffer, BUF_SIZE, 0)) > 0); // read data from input socket

  shutdown(source_sock, SHUT_RDWR); // stop other processes from using socket
  close(source_sock);
}

/**
   Make file descriptor non blocking
*/
int setNonblocking(int fd)
{
  int flags;

  /* If they have O_NONBLOCK, use the Posix way to do it */
#if defined(O_NONBLOCK)
  /* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
  if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
    flags = 0;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
  /* Otherwise, use the old way of doing it */
  flags = 1;
  return ioctl(fd, FIOBIO, &flags);
#endif
}     

/**
   set buffer size to given value
*/

void setBufferSize(int fd, int buffer)
{
  DEBUG_PRINT("pipe size before %d \n", fcntl(fd,F_GETPIPE_SZ));

  if(fcntl(fd, F_SETPIPE_SZ, buffer)<0)
    {
      perror("Error in setting pipe size");
    }

  DEBUG_PRINT("pipe size after %d \n", fcntl(fd,F_GETPIPE_SZ));  
}

/**
   get pipe Buffer size
*/

void getPipeSize(int pipe_in, int pipe_out){
  int buf_in_size;
  int buf_out_size;

  if(ioctl(pipe_in, FIONREAD, &buf_in_size)<0)
    {
      perror("Error \n");
      exit(0);
    }
  printf(" Pipe Buffer IN Size %d \n", (buf_in_size));

  if(ioctl(pipe_out, FIONREAD, &buf_out_size)<0)
    {
      perror("Error \n");
      exit(0);
    }

  printf(" Pipe Buffer OUT Size %d \n", (buf_out_size));
}


/*
  void getPipeSize2(int pipe_in, int pipe_out){

  struct stat sb;

  if (fstat(pipe_in, &sb) == -1) {
  perror("stat");
  exit(EXIT_FAILURE);
  }
  printf("File size 1: %lld bytes\n", (long long) sb.st_size);


  if (fstat(pipe_out, &sb) == -1) {
  perror("stat");
  exit(EXIT_FAILURE);
  }
  printf("File size 2: %lld bytes\n", (long long) sb.st_size);
  }
*/

void print_timeofday(){
  char buffer[30];
  struct timeval tv;
  time_t curtime;

  curtime=tv.tv_sec;

  strftime(buffer,30,"%m-%d-%Y  %T.",localtime(&curtime));
  DEBUG_PRINT("%s%ld\n",buffer,tv.tv_usec);
}
