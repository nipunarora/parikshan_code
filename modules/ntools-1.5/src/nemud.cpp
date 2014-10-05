/*************************************************************************

 nemud.cpp
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/time.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>
#include <errno.h>

#include "defs.h"
#include "utils.h"
#include "linkedlist.h"
#include "impairs.h"

using namespace std;

// function declarations

void cleanup( int );
void usage();
void emulator_rt();  // real-time emulator
void emulator_aw();  // active waiting emulator
void *sender( void * );

// global variables and definitions

struct Data
{
	struct timeval sendtime;
	int len;
	void *frame;
};

LinkedList <Data> frames[2];
int sock[2];
char ifname[2][IFNAMSIZ];
int smid;
Impairs *impairs;

// the main function

int main( int argc, char **argv )
{
	int rt;  // whether the kernel is rt capable
	int i;
	struct sockaddr_ll addrll;
	struct sched_param sch_param;
	int par_if1, par_if2;
	sigset_t sigset;
	struct sigaction act;

	cerr << "nemud " << VERSION;

	// process the command line arguments
	
	if( argc != 5 ) usage();
	par_if1 = par_if2 = 0;
	
	rt = checkRT();
	
	try
	{
		while( argc > 1 )
		{
			if( !strcmp( "-1if", argv[1] ) )
			{
				if( par_if1 ) throw "1if parameter is duplicated";
				if( argc == 1 ) usage();
				strncpy( ifname[0], argv[2], IFNAMSIZ );
				ifname[0][IFNAMSIZ-1] = 0;
				par_if1 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-2if", argv[1] ) )
			{
				if( par_if2 ) throw "2if parameter is duplicated";
				if( argc == 1 ) usage();
				strncpy( ifname[1], argv[2], IFNAMSIZ );
				ifname[1][IFNAMSIZ-1] = 0;
				par_if2 = -1;
				argc -= 2; argv += 2;
			}
			else
			{
				throw "Invalid option";
			}
		}
		
		// check parameters
		
		if( par_if1 == 0 ) throw "if1 parameter is missing";
		if( par_if2 == 0 ) throw "if2 parameter is missing";
	}
	catch( char const *e )
	{
		cerr << e << endl;
		return -1;
	}
	
	// set signal handlers
	sigemptyset( &sigset );
	sigaddset( &sigset, SIGALRM );
	pthread_sigmask ( SIG_BLOCK, &sigset, NULL );
	
	memset( &act, 0, sizeof( act ) );
	act.sa_handler = cleanup;
	if( sigaction( SIGINT, &act, NULL ) )
	{
		perror( "Cannot set signal handler for SIGINT" );
		exit( -1 );
	}
	if( sigaction( SIGTERM, &act, NULL ) )
	{
		perror( "Cannot set signal handler for SIGTERM" );
		exit( -1 );
	}
	
	// create shared memory and initalize it
	
	smid = shmget( SMKEY, 2*sizeof( Impairs ), IPC_CREAT );
	if( smid == -1 )
	{
		perror( "Cannot create shared memory" );
		return -1;
	}
	impairs = ( Impairs * )shmat( smid, NULL, 0 );
	impairs[0].Init();
	impairs[1].Init();
	frames[0].threadSafe();
	frames[1].threadSafe();

	// create sockets

	for( i = 0; i  < 2; i++ )
	{ 
		sock[i] = socket( PF_PACKET, SOCK_RAW, htons( ETH_P_ALL ) );  // packet socket to capture everything
		if( sock[i] == -1 )
		{
			cerr << "Cannot create the packet socket for " << ifname[i] << endl;
			perror( "" );
			exit( -1 );
		}
		if( setPromiscMode( ifname[i], -1 ) ) exit( -1 );
		try
		{
			addrll.sll_ifindex = getIfIndex( ifname[i] );
		}
		catch( char const *e )
		{
			perror( e );
			exit( -1 );
		}
		addrll.sll_family = AF_PACKET;
		if( bind( sock[i], ( struct sockaddr * )&addrll, sizeof( addrll ) ) )
		{
			cerr << "Cannot bind socket for " << ifname[i] << endl;
			perror( "" );
			exit( -1 );
		}
	}
	
	if( rt )
	{
		sch_param.sched_priority = 99;
		if(  sched_setscheduler( 0, SCHED_FIFO, &sch_param ) )
		{
			perror( "Cannot set priority scheduling" );
			exit( -1 );
		}
		emulator_rt();
	}
	else
	{
		emulator_aw();
	}
}

/////////////////////////////////////////////////////////////////////////////////////
void emulator_rt()
{
	void *framep;
	Data *datap;
	char buf[1600];
	int buflen = 1600;  // should accomodate any valid frames
	int i, msglen, empty;
	int d1, d2, fjitter;
	struct timeval now;
	struct JittLoss jittloss;
	fd_set set;
	int nfds;
	int i0, i1;
	pthread_t tid[2];
	
	if( sock[0] > sock[1] )
	{
		nfds = sock[0] + 1;
	}
	else
	{
		nfds = sock[1] + 1;
	}
	
	// start the sender thread
	i0 = 0;
	i1 = 1;
	if( pthread_create( &tid[0], NULL, sender, ( void * )&i0 ) != 0 )
	{
		cerr << "Cannot create a sender thread\n";
		exit( -1 );
	}
	if( pthread_create( &tid[1], NULL, sender, ( void * )&i1 ) != 0 )
	{
		cerr << "Cannot create a sender thread\n";
		exit( -1 );
	}
	
	// main read cycle
	while( -1 )
	{
		FD_ZERO( &set );
		FD_SET( sock[0], &set );
		FD_SET( sock[1], &set );
		
		select( nfds, &set, NULL, NULL, NULL );
		
		for( i = 0; i < 2; i++ )
		{
			if( FD_ISSET( sock[i], &set ) )
			{
				msglen = recv( sock[i], buf, buflen, 0 );
				if( msglen < 1 )  // reading error
				{
					if( errno != EAGAIN )
					{
						cerr << "Reading error on " << ifname[i] << endl;
						perror( "" );
						exit( -1 );
					}
				}
				else
				{
					gettimeofday( &now, NULL );
					impairs[i].total++;
					fjitter = 0;
					if( impairs[i].closs.exist )
					{
						d1 = impairs[i].closs.Drop();
					}
					else
					{
						d1 = 0;
					}
					if( impairs[i].bloss.exist )
					{
						d2 = impairs[i].bloss.Drop( now );
					}
					else
					{
						d2 = 0;
					}
					if( impairs[i].jitter.exist )
					{
						jittloss = impairs[i].jitter.Value( now );
					}
					else
					{
						jittloss.loss = 0;
						jittloss.jitter = 0;
					}
					if( d1 || d2 || jittloss.loss || ( fjitter == -1 ) )  // implement loss
					{
						impairs[i].dropped++;
					}
					else
					{
						framep = malloc( msglen );
						if( framep == NULL )
						{
							cerr << "Cannot allocate memory\n";
							exit( -1 );
						}
						memcpy( framep, ( void * ) buf, msglen );
						datap = new Data;
						if( datap == 0 )
						{
							cerr << "Cannot allocate memory\n";
							exit( -1 );
						}
						datap->sendtime = now;
						if( impairs[i].delay != 0 ) datap->sendtime += impairs[i].delay;  // implement delay
						if( impairs[i].cjitter.exist )
						{
							datap->sendtime += impairs[i].cjitter.Value();  // implement continuous jitter
						}
						if( jittloss.jitter ) datap->sendtime += jittloss.jitter;  // implement jitter
						datap->frame = framep;
						datap->len = msglen;
						frames[i].wrlock();
						if( frames[i].empty() )
						{
							empty = -1;
						}
						else
						{
							empty = 0;
						}
						frames[i].insertAfter( datap );
						frames[i].unlock();
						if( empty )  // signal the sender thread that there are frames now
						{
							pthread_kill( tid[i], SIGALRM );
						}
					}
				}
			}
		}
	}
}

void *sender( void *arg )
{
	int x, i;
	long wait;
	Data *datap[2];
	struct timeval now;
	sigset_t sigset;
	
	i = *( int * ) arg;   // the interface we work on 0 or 1
	sigemptyset( &sigset );
	sigaddset( &sigset, SIGALRM );
	while( -1 )
	{
		datap[i] = frames[i].first();
		if( datap[i] )
		{
			gettimeofday( &now, 0 );
			wait = timeDiff( datap[i]->sendtime, now );
		}
		else
		{
			// no frames, we need to wait until we got something
			sigwait( &sigset, &x );
			continue;
		}
		if( wait > WAITING_TIMELIMIT )
		{
			usleep( wait );
		}
		// now send the frame
		if( send( sock[1-i], datap[i]->frame, datap[i]->len, 0 ) < 0 )
		{
			cerr << "Send error on " << ifname[1-i] << endl;
			perror( "" );
			exit( -1 );
		}
		else
		{
			free( datap[i]->frame );
			frames[i].wrlock();
			frames[i].remove();
			frames[i].unlock();
		}
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////
void emulator_aw()
{
	int i;
	void *framep;
	Data *datap;
	char buf[1600];
	int buflen = 1600;  // should accomodate any valid frames
	int msglen;
	int d1, d2, fjitter;
	struct timeval now;
	struct JittLoss jittloss;
	
	while( -1 )
	{
		for( i = 0; i < 2; i++ )
		{
			msglen = recv( sock[i], buf, buflen, MSG_DONTWAIT );
			if( msglen < 1 )  // reading error
			{
				if( errno != EAGAIN )
				{
					perror( "Reading error on the first interface\n" );
				}
			}
			else
			{
				gettimeofday( &now, NULL );
				impairs[i].total++;
				fjitter = 0;
				if( impairs[i].closs.exist )
				{
					d1 = impairs[i].closs.Drop();
				}
				else
				{
					d1 = 0;
				}
				if( impairs[i].bloss.exist )
				{
					d2 = impairs[i].bloss.Drop( now );
				}
				else
				{
					d2 = 0;
				}
				if( impairs[i].jitter.exist )
				{
					jittloss = impairs[i].jitter.Value( now );
				}
				else
				{
					jittloss.loss = 0;
					jittloss.jitter = 0;
				}
				if( d1 || d2 || jittloss.loss || ( fjitter == -1 ) )  // implement loss
				{
					impairs[i].dropped++;
				}
				else
				{
					framep = malloc( msglen );
					if( framep == NULL )
					{
						cerr << "Cannot allocate memory\n";
						exit( -1 );
					}
					memcpy( framep, ( void * ) buf, msglen );
					datap = new Data;
					if( datap == 0 )
					{
						cerr << "Cannot allocate memory\n";
						exit( -1 );
					}
					datap->sendtime = now;
					if( impairs[i].delay != 0 ) datap->sendtime += impairs[i].delay;  // implement delay
					if( impairs[i].cjitter.exist )
					{
						datap->sendtime += impairs[0].cjitter.Value();  // implement continuous jitter
					}
					if( jittloss.jitter ) datap->sendtime += jittloss.jitter;  // implement jitter
					datap->frame = framep;
					datap->len = msglen;
					frames[0].last();
					frames[0].insertAfter( datap );
				}
			}
		}
		
		for( i = 0; i < 2; i++ )
		{
			gettimeofday( &now, NULL );
			frames[i].first();
			while( !frames[i].end() )
			{
				datap = frames[i].get();
				if( ( now > datap->sendtime ) )
				{
					if( send( sock[i], datap->frame, datap->len, 0 ) < 0 )
					{
						cerr << "Send error on " << ifname[i] << endl;
						perror( "" );
					}
					else
					{
						free( datap->frame );
						frames[i].remove();
						continue;
					}
				}
				break;
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////////////////////
void cleanup( int sig )
{
	shmctl( smid, IPC_RMID, NULL );
	shmdt( impairs );
	exit( 0 );
}


/////////////////////////////////////////////////////////////////////////////////////
void usage()
{
	cerr << "Command line arguments:\n";
	cerr << "-1if input_interface\n";
	cerr << "-2if output_interface\n";
	exit( -1 );
}
