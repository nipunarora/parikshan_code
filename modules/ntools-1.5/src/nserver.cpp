/*************************************************************************

 nserver.cpp
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/

#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <exception>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include "defs.h"
#include "utils.h"

using namespace std;

void usage();
void *sender( void * );

int g_tos;  // the IP precedence value
char g_buf[65536];  // the common sending buffer

int main( int argc, char **argv )
{
	int i;
	int port, accsock, sock;
	unsigned char c;
	pthread_t tid;
	pthread_attr_t attr;
	struct sockaddr_in local;
	int *newsocketp;
	int par_port, par_tos;
		
	cerr << "nserver " << VERSION;

	// set the parameters to the default values
	
	par_port = par_tos = 0;
	port = DEF_SPORT;
	g_tos = 0;  // the default precedence value

	// process the command line arguments

	if( argc == 2 ) usage();
	try
	{
		while( argc != 1 )
		{
			if( !strcmp( "-port", argv[1]  ) )
			{	
				if( par_port ) throw "ERR: port parameter is duplicated";
				if( argc == 1 ) usage();
				port = atoi( argv[2] );
				if( port < 0 || port > 65545 ) throw "ERR: invalid port";
				par_port = -1;
			}
			else if( !strcmp( "-prec", argv[1] ) )
			{	
				if( par_tos ) throw "ERR: prec/tos parameter is duplicated";
				if( argc == 1 ) usage();
				g_tos = atoi( argv[2] );
				if( g_tos < 0 || g_tos > 7 ) throw "ERR: invalid prec";
				g_tos <<= 5;
				par_tos = -1;
			}
			else if( !strcmp( "-tos", argv[1] ) )
			{	
				if( par_tos ) throw "ERR: prec/tos parameter is duplicated";
				if( argc == 1 ) usage();
				g_tos = atoi( argv[2] );
				if( g_tos < 0 || g_tos > 255 ) throw "ERR: invalid tos";
				par_tos = -1;
			}
			else
			{
				throw string( "ERR: invalid option: " ) + argv[1];
			}
			argc -= 2;
			argv += 2;
		}
	}
	catch( char const *e )
	{
		cerr << e << endl;
		return -1;
	}
	catch( string e )
	{
		cerr << e << endl;
		return -1;
	}
	
	pthread_attr_init( &attr );
	
	// we have to set the thread to detached, otherwise the resources of the
	// finished threads quickly fills the available kernel space
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
	
	// now wait for incoming connections
	
	accsock = socket( PF_INET, SOCK_STREAM, 0 );
	if( accsock == -1 )
	{
		perror( "ERR: cannot open socket" );
		exit( -1 );
	}
	i = -1;
	if( setsockopt( accsock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof( i ) ) )
	{
		perror( "ERR: cannot set SO_REUSEADDR socket option" );
	}
	c = g_tos;
	if( setsockopt( accsock, SOL_IP, IP_TOS, ( void * )&c, sizeof( c ) ) )
	{
		perror( "ERR: cannot set TOS for the socket" );
		exit( -1 );
	}
	local.sin_family = PF_INET;
	local.sin_port = htons( port );
	local.sin_addr.s_addr = htonl( INADDR_ANY );
	i = DEF_WINDOW;
	if( setsockopt( accsock, SOL_SOCKET, SO_SNDBUF, &i, sizeof( i ) ) )
	{
		perror( "ERR: cannot set SO_SNDBUF socket option" );
	}
	if( bind( accsock, ( struct sockaddr * )&local, sizeof( local ) ) == -1 )
	{
		perror( "ERR: cannot bind socket" );
		exit( -1 );
	}
	if( listen( accsock, 256 ) == -1 )
	{
		perror( "ERR: cannot listen on socket" );
		exit( -1 );
	}
	while( -1 )
	{
		sock = accept( accsock, NULL, NULL );
		if( sock == -1 )
		{
      		perror( "ERR: cannot accept" );
      		exit( -1 );
		}
		
		// create a new thread for this connection, pass the socket as argument

		newsocketp = new int;
		*newsocketp = sock;
		if( pthread_create( &tid, &attr, sender, ( void * )newsocketp ) != 0 )
		{
			cerr << "ERR: cannot create a server thread\n";
			exit( -1 );
		}
	}
}
		
// the server thread

void *sender( void *arg )
{
	int sock;
	int n, x;
	char mybuf[10];
	unsigned long len;
	
	sock = *( int * )arg;  // the accepted socket
	delete ( int * )arg;
	n = recv( sock, mybuf, sizeof( mybuf ), 0 );
	if( n < 1 )
	{
		perror( "ERR: reading error\n" );
		pthread_exit( NULL );
	}
	len = ntohl( *( unsigned long * )mybuf );  // read the bytes to send
	
	// send the read bytes
	
	while( len != 0 )
	{
		if( len >= 65536 ) x = 65536;
		else x = len;
		if( send( sock, g_buf, x, MSG_NOSIGNAL ) == -1 )
		{
			perror( "ERR: send error" );
			pthread_exit( NULL );
		}
		len -= x;
	}
	close( sock );
	return NULL;
}

void usage()
{
	cerr << "Command line arguments:\n";
	cerr << "[-port port] : the default is " << DEF_SPORT << "\n";
	cerr << "[-prec precedence]\n";
	cerr << "[-tos tos]\n";
	exit( -1 );
}
