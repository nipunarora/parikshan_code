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
#include "input.h"
#include "queue.h"
#include "network_comm.h"
#include "shm_incr.h"
#include "pipe_input.h"

bool l, h, p_flag, x, d, a, s, b = FALSE;
bool asynch = FALSE, synch = FALSE;
int pipes_arr[NUM_CONNS][2];

void server_loop();
void handle_client(int client_sock, struct sockaddr_in client_addr);

void handle_prod_client(int client_sock, struct sockaddr_in client_addr);
void forward_data(int source_sock, int destination_sock);
void forward_data_to_dest_and_pipe(int source_sock, int destination_sock, int pipe);

void print_timeofday();
void create_pipes();
  
/* Program start */
int main(int argc, char *argv[]) {
  int c, local_port;
  pid_t pid;
  
  local_port = parse_options(argc, argv);

  if (local_port < 0){
    printf("Syntax: %s -l local_port -h remote_host -p remote_port -x duplicate_host -d duplicate_port -a|s (a= asynchrounous mode, s = synchrounous mode) -b buffer_size \n", argv[0]);
    return 0;
  }

  /* Here we start the proxy server with it listening at the local port specified */
  if ((server_sock = create_socket(local_port)) < 0) { // start server
    perror("Cannot run server");
    return server_sock;
  }

  signal(SIGCHLD, sigchld_handler); // prevent ended children from becoming zombies
  signal(SIGTERM, sigterm_handler); // handle KILL signal

  /* Here we fork of a daemon which starts the server_loop function */
  switch(pid = fork()){
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

void create_pipes(){
  int i;
  for (i=0; i<NUM_CONNS;i++){
    
    if(pipe(pipes_arr[i])<0){
      perror("pipe");
      exit(1);
    }
    
    setNonblocking(pipes_arr[i][1]);
//  setBufferSize(pipes_arr[i][1],buffer_size);
    
  }
}

/* Parse command line options */
int parse_options(int argc, char *argv[]) {
	
  int c, local_port;
  l = h = p_flag = x = d = FALSE;

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
          p_flag = TRUE;
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
  if (l && h && p_flag && !a && !s) {
    return local_port;
  } 
  if (l && h && p_flag && (a||s) && x && d){
    return local_port;
  }
  else {
    return -1;
  }
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

  create_pipes();

  while (TRUE){
    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addrlen);    	
    if (fork() == 0){ // handle client connection in a separate process
      close(server_sock);
      handle_client(client_sock, client_addr);
      exit(0);
    }	
    close(client_sock);
  }
}


void handle_client(int client_sock, struct sockaddr_in client_addr){
  
  char temp[INET6_ADDRSTRLEN];
  int temp_port;

  getpeerinfo(client_sock, &temp[0], &temp_port);

  if(strcmp(&temp[0], remote_host)==0){
    handle_prod_client(client_sock, client_addr);
  }

}

// ---> 
void handle_prod_client(int client_sock, struct sockaddr_in client_addr){

  if ((remote_sock = create_connection()) < 0) {
    perror("Cannot connect to host");
    return;
  }

  int c = get_shm_counter();
  increment_shm_counter();

  if (fork() == 0) { // a process forwarding data from client to
    forward_data(client_sock, remote_sock);
  }

  if (fork() == 0) { // a process forwarding data from remote socket
    forward_data_to_dest_and_pipe(remote_sock, client_sock, pipes_arr[c][1]);
  }
}

/* Forward data between sockets */
void forward_data(int source_sock, int destination_sock) {
  char buffer[BUF_SIZE];
  int n;

  while ((n = recv(source_sock, buffer, BUF_SIZE, 0)) > 0) { // read
    send(destination_sock, buffer, n, 0); // send data to output
  }

  shutdown(destination_sock, SHUT_RDWR); // stop other processes from
  close(destination_sock);

  shutdown(source_sock, SHUT_RDWR); // stop other processes from using
  close(source_sock);
}

void forward_data_to_dest_and_pipe(int source_sock, int destination_sock, int pipe){
  char buffer[BUF_SIZE];
  int n;

  while ((n = recv(source_sock, buffer, BUF_SIZE, 0)) > 0) { // read
    send(destination_sock, buffer, n, 0); // send data to output
    if(write(pipe,buffer,n)<0){
      perror("Buffer Overflow \n");
    }
  }

  shutdown(destination_sock, SHUT_RDWR); // stop other processes from
  close(destination_sock);

  shutdown(source_sock, SHUT_RDWR); // stop other processes from using
  close(source_sock);
}


// ---> HANDLE DUPLICATE CLIENT

void handle_prod_client(int client_sock){

  if (fork() == 0) { // a process forwarding data from client to
    read_data_and_drop(client_sock);
  }

  if (fork() == 0) { // a process forwarding data from remote socket
    forward_data_to_dest_and_pipe(remote_sock, client_sock, pipes_arr[c][1]);
  }
}
