/* 
 * 
 * Filename: network_comm.c
 * Author: Nipun Arora
 * Created: Sun Dec 28 12:46:52 2014 (-0500)
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
#include "network_comm.h"

/* Create server socket this is socket at which the proxy is binded and listens for incoming connections */
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


int create_connection(char *destination_host, int destination_port) {
  struct sockaddr_in server_addr;
  struct hostent *server;
  int sock;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    return CLIENT_SOCKET_ERROR;
  }

  if ((server = gethostbyname(destination_host)) == NULL) {
    errno = EFAULT;
    return CLIENT_RESOLVE_ERROR;
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  server_addr.sin_port = htons(destination_port);

  if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
    return CLIENT_CONNECT_ERROR;
  }

  return sock;
}

/*  get peername info */
void getpeerinfo(int socket, char ipstr[], int* port){

  struct sockaddr_storage addr;
  socklen_t len;
  len = sizeof(addr);
  //char ipstr[INET6_ADDRSTRLEN];
  int connection_counter=0;

  getpeername(socket, (struct sockaddr*)&addr, &len);
  // deal with both IPv4 and IPv6:
  if (addr.ss_family == AF_INET){
    struct sockaddr_in *s = (struct sockaddr_in *)&addr;
    *port = ntohs(s->sin_port);
    inet_ntop(AF_INET, &s->sin_addr, ipstr, INET6_ADDRSTRLEN);
  } 
  else{ // AF_INET6
    struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
    *port = ntohs(s->sin6_port);
    inet_ntop(AF_INET6, &s->sin6_addr, ipstr, INET6_ADDRSTRLEN);
  }

  printf("\n Peer IP address: %s\n", ipstr);
  printf("\n Peer port      : %d\n", *port);
  
  //*temp = &ipstr[0];
}
