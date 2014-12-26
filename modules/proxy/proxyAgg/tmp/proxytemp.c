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
//#include <wait.h>
#include <fcntl.h>
#include <pthread.h>

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
int pipe_desc_prod[1000][2];
int pipe_desc_prod[1000][2];
int counter_prod = 0;
int counter_dup = 0;

/* Program start */
int main(int argc, char *argv[]) 
{

  int c, local_port;
  pid_t pid;

  int server_sock, local_port, remote_port;
  char *remote_host, *prod_source, *dup_source;


  local_port = parse_options(argc, argv, remote_host, prod_source, dup_source, local_port, remote_port);
  if (local_port < 0) {
    printf("Syntax: %s -l local_port -h remote_host -p remote_port -x prod_source -y dup_source \n", argv[0]);
    return 0;
  }
  DEBUG_PRINT ("local_port: %d remote_port %d remote_host %s prod_source %s dup_source %s \n", local_port,remote_port, )


    if ((server_sock = create_socket(local_port)) < 0) { // start server
      perror("Cannot run server");
      return server_sock;
    }

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
int parse_options(int argc, char *argv[], char *remote_host, char *prod_source, char *dup_source, int &local_port, int &remote_port) ;
{

  bool l, h, p;
  int c, local_port;

  l = h = p = FALSE;

  while ((c = getopt(argc, argv, "l:h:p:")) != -1) 
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
          prod_source = optarg;
          x = TRUE;
          break;
        case 'y':
          dup_source = optarg;
          y = TRUE;
          break;
        }
    }

  if (l && h && p) 
    {
      return local_port;
    } 
  else 
    {
      return -1;
    }
}

/* Create server socket */
int create_socket(int port) 
{

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

  if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) 
    {
      return SERVER_BIND_ERROR;
    }

  if (listen(server_sock, 20) < 0) 
    {
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
  exit(0);
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


  //Compare IPSTR and PRODUCTION SOURCE
  if(strcmp (ipstr,prod_source) != 0)
    {
      prod_flag=TRUE;
    }
  else
    {
      if(strcmp(ipstr,dup_source)!=0)
        {
          prod_flag=FALSE;
        }       
      else
        {
          printf("We have a problem\n");
        }
    }
 
  printf("Peer IP address: %s\n", ipstr);
  printf("Peer port      : %d\n", port);

  // set counter for connection number
  connection_counter++;
}

void initialize_pipe()
{
  if(pipe(pfds[connection_counter])<0)
    {
      perror("pipe ");
      exit(1);
    }

  setNonblocking(pfds[connection_counter][1]);
  DEBUG_PRINT("pipe size %d \n",fcntl(pfds[connection_counter][1],F_GETPIPE_SZ));    
    
  if(b)
    {
      setBufferSize(pfds[connection_counter][1],buffer_size);
      //setBufferSize(pfds[0],buffer_size);
    }      
}

int *new_sock;

/* Main server loop */
void server_loop() 
{
  struct sockaddr_in client_addr;
  int addrlen = sizeof(client_addr);
  int client_sock;

  while (TRUE) 
    {
      client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addrlen);
      if(client_sock<0)
        {
          perror("accept failed");
          return 1;
        }

      new_sock = malloc(1);
      *new_sock = client_sock;

      pthread_t thread1;
      if(pthread_create( &thread1, NULL, handle_client, (void*) new_sock) < 0)
        {
          perror("Could not create thread");
          return 1;
        }
    }

}

/* Handle client connection */
void *handle_client(void *arguments)
{
  int client_sock = *(int *)arguments;
  int remote_sock;
  // handle client connection in a separate process
  getpeerinfo(client_sock);   
}

/*
//COMMUNICATION WITH THE PRODUCTION --> START

// Forward data from production container to backend 
void forward_data(int source_sock, int destination_sock) 
{
char buffer[BUF_SIZE];
int n;

while ((n = recv(source_sock, buffer, BUF_SIZE, 0)) > 0) 
{ // read data from input socket
send(destination_sock, buffer, n, 0); // send data to output socket

}

shutdown(destination_sock, SHUT_RDWR); // stop other processes from using socket
close(destination_sock);

shutdown(source_sock, SHUT_RDWR); // stop other processes from using socket
close(source_sock);
}

//receive data and send to pipe, and the production container
void receive_data_asynchronous(int source_sock, int destination_sock) 
{
char buffer[BUF_SIZE];
int n;

while ((n = recv(source_sock, buffer, BUF_SIZE, 0)) > 0) 
{ // read data from input socket
send(destination_sock, buffer, n, 0); // send data to output socket
}

shutdown(destination_sock, SHUT_RDWR); // stop other processes from using socket
close(destination_sock);

shutdown(source_sock, SHUT_RDWR); // stop other processes from using socket
close(source_sock);
}

//COMMUNICATION WITH THE PRODUCTION --> END

//COMMUNICATION WITH THE DUPLICATE --> START

// Forward data from duplicate container to nowhere 
void forward_data_drop(int source_sock, int destination_sock) 
{
char buffer[BUF_SIZE];
int n;

while ((n = recv(source_sock, buffer, BUF_SIZE, 0)) > 0);

shutdown(destination_sock, SHUT_RDWR); // stop other processes from using socket
close(destination_sock);

shutdown(source_sock, SHUT_RDWR); // stop other processes from using socket
close(source_sock);
}

//receive data from pipe and send to duplicate container
void receive_pipe_data(int source_sock, int destination_sock) 
{
char buffer[BUF_SIZE];
int n;

while ((n = recv(source_sock, buffer, BUF_SIZE, 0)) > 0) 
{ // read data from input socket
send(destination_sock, buffer, n, 0); // send data to output socket
}

shutdown(destination_sock, SHUT_RDWR); // stop other processes from using socket
close(destination_sock);

shutdown(source_sock, SHUT_RDWR); // stop other processes from using socket
close(source_sock);
}

//COMMUNICATION WITH THE DUPLICATE END

// Create client connection 
int create_connection() 
{
struct sockaddr_in server_addr;
struct hostent *server;
int sock;

if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
{
return CLIENT_SOCKET_ERROR;
}

if ((server = gethostbyname(remote_host)) == NULL) 
{
errno = EFAULT;
return CLIENT_RESOLVE_ERROR;
}

memset(&server_addr, 0, sizeof(server_addr));
server_addr.sin_family = AF_INET;
memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
server_addr.sin_port = htons(remote_port);

if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) 
{
return CLIENT_CONNECT_ERROR;
}

return sock;
}
*/
