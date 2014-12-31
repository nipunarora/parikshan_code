/* 
 * 
 * Filename: network_comm.h
 * Author: Nipun Arora
 * Created: Sun Dec 28 12:46:41 2014 (-0500)
 * URL: http://www.nipunarora.net 
 * 
 * Description: 
 */

#ifndef NETWORK_COMM_H
#define NETWORK_COMM_H

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

int buffer_size;
int server_sock, client_sock, remote_sock,duplicate_destination_sock;


int create_socket(int port);
int create_connection(char* destination_host, int destination_port);
void getpeerinfo(int s, char ipstr[], int* port);

#endif
