/*************************************************************************

 nclient.cpp
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/

#include <string>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <exception>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "defs.h"
#include "utils.h"
#include "distrib.h"

using namespace std;

void usage();
void *receiver( void * );
void *starter( void * );

int g_tos;  // the IP precedence value
int g_port;  // the TCP port
int g_max;  // the maximum number of allowed threads
char g_buf[65536];  // the common receiving buffer
unsigned long g_waiting;  // the average waiting time between connections
unsigned long g_ip;  // the address of the server
unsigned long g_distArray[MAX_ARRSIZE];  // the distribution array for the file sizes
int g_arrsize;  // the size of g_distArray
int g_threads;  // the number of running threads
pthread_mutex_t g_thread_mutex;  // protects g_threads
long g_bytes;

struct Statistics  // holds tstatistics for file sizes
{
	unsigned long size;  // file size
	unsigned long num;  // number of downloads in the interval
	struct timeval time;  // sum of download times in the interval
	pthread_mutex_t mutex;  // mutex to protect this struct
};

Statistics g_downloads[MAX_FILESIZES+1];  // the array for download statitics

int main( int argc, char **argv )
{
	int i, n;
	ifstream distfile;  // the distribution file
	char distfname[256];
	int writestat;  // indicates that we have to write statistics to file
	char statfname[256];
	ofstream statfile;  // the output file for writing statistics
	ostringstream ostream;  // output stream buffer
	unsigned long fsize, x, sec, timediff;
	unsigned long interval;  // the statistical interval time in sec
	float avgtime, rate;
	int freq, files;
	sigset_t sigset;
	pthread_t tid;
	Statistics *mystat;
	struct timeval time, now, lastStat;
	struct itimerval itimer;
	struct in_addr inaddr;
	int par_ip, par_port, par_file, par_max;
	int par_wait, par_stat, par_int, par_tos;
	
	cerr << "nclient " << VERSION;

	// set the parameters to the default values

	interval = 1;   // default statistical interval is 1 sec
	writestat = 0;
	g_tos = 0;  // the default precedence value
	g_max = DEF_MAX_NUM_OF_NCLIENTS;
	g_port = DEF_SPORT;
	g_waiting = DEF_WAIT;
	g_bytes = 0;
	strncpy( distfname, DEF_FILE, 255 );
	par_ip = par_port = par_file = par_max = 0;
	par_wait = par_stat = par_int = par_tos = 0;
	
	// process the command line arguments
	
	if( argc < 3 ) usage();
	par_ip =  0;
	try
	{
		while( argc != 1 )
		{
			if( !strcmp( "-ip", argv[1] ) )
			{
				if( par_ip ) throw "ERR: ip parameter is duplicated";
				if( argc == 1 ) usage();
				if( inet_aton( argv[2], &inaddr ) == 0 ) throw "ERR: invalid IP address: ";
				g_ip = inaddr.s_addr;
				par_ip = -1;
			}
			else if( !strcmp( "-port", argv[1] ) )
			{	
				if( par_port ) throw "ERR: port parameter is duplicated";
				if( argc == 1 ) usage();
				g_port = atoi( argv[2] );
				if( g_port < 0 || g_port > 65545 ) throw "ERR: Invalid port";
				par_port = -1;
			}
			else if( !strcmp( "-max", argv[1] ) )
			{	
				if( par_max ) throw "ERR: max parameter is duplicated";
				if( argc == 1 ) usage();
				g_max = atoi( argv[2] );
				if( g_max < 1 || g_max > 100 ) throw "ERR: invalid max paramter, must be between 1 and 100";
				par_max = -1;
			}
			else if( !strcmp( "-file", argv[1] ) )
			{
				if( par_file ) throw "ERR: file parameter is duplicated";
				if( argc == 1 ) usage();
				strncpy( distfname, argv[2], 255 );
				par_file = -1;
			}
			else if( !strcmp( "-wait", argv[1] ) )
			{	
				if( par_wait ) throw "ERR: wait parameter is duplicated";
				if( argc == 1 ) usage();
				g_waiting = readTime( argv[2] ) / 1000;
				if( g_waiting < 1 || g_waiting > 2000 ) throw "ERR: invalid wait parameter, must be between 1 ms and 2 s";
				par_wait = -1;
			}
			else if( !strcmp( "-prec", argv[1] ) )
			{	
				if( par_tos ) throw "ERR: prec/tos parameter is duplicated";
				if( argc == 1 ) usage();
				g_tos = atoi( argv[2] );
				if( g_tos < 0 || g_tos > 7 ) throw "ERR: invalid precedence";
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
			else if( !strcmp( "-int", argv[1] ) )
			{
				if( par_int ) throw "ERR: int parameter is duplicated";
				if( argc == 1 ) usage();
				interval = atoi( argv[2] );
				if( interval <= 0 ) throw "ERR: invalid time interval";
				par_int = -1;
			}
			else if( !strcmp( "-stat", argv[1] ) )
			{
				if( par_stat ) throw "ERR: stat parameter is duplicated";
				if( argc == 1 ) usage();
				strcpy( statfname, argv[2] );
				writestat = -1;
				par_stat = -1;
			}
			else
			{
				throw string( "ERR: invalid option: " ) + argv[1];
			}
			argc -= 2;
			argv += 2;
		}
		if( !par_ip ) throw "ERR: the ip parameter is missing";
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
	
	// now open the distribution file and process it
	
	distfile.open( distfname, ios::in );
	if( !distfile )
	{
		cerr << "ERR: cannot open " << distfname << endl;
		exit( -1 );
	}
	g_arrsize = 0;
	files = 1;  // the number of processed file sizes + 1

	// g_downloads[0] is used for the incomplete downloads

	while( !nextWord( distfile, g_buf, 256 ) )
	{
		fsize = strtoul( g_buf, NULL, 0 );  // read the file size
		if( nextWord( distfile, g_buf, 256 ) )
		{
			cerr << "ERR: error in the distribution file\n";
			exit( -1 );
		}
		freq = atoi( g_buf );  // the relative probability for this size

		// now add this file size to the distArray freq times

		for( i = 0; ( i < freq ) && ( g_arrsize < MAX_ARRSIZE ); i++, g_arrsize++ )
		{
			g_distArray[g_arrsize] = files;  // an index to the g_downloads array
		}
		g_downloads[files].size = fsize;  // set the file size in g_downloads
		files++;
		if( files == MAX_FILESIZES + 1 ) break;
	}

	// initialization

	pthread_mutex_init( &( g_thread_mutex ), NULL );
	for( i = 0; i < files; i++ )
	{
		mystat = g_downloads + i;
		mystat->num = 0;
		mystat->time.tv_sec = 0;
		mystat->time.tv_usec = 0;
		pthread_mutex_init( &( mystat->mutex ), NULL );
	}
	
	// open the stat file
	
	if( writestat )
	{
		statfile.open( statfname, ios::out );
		if( !statfile )
		{
			cerr << "ERR: cannot open file " << statfname << endl;
			exit( -1 );
		}
	}

	ostream.str( "" );
	
	ostream << setw( 10 ) << "Size" << setw( 8 ) << "DLoads" << \
	setw( 10 ) << "DTime" << endl << ends;
	cout << ostream.str();
	if( writestat ) statfile << ostream.str();

	// block SIGALRM and SIGINT, needed for sigwait
	
	sigemptyset( &sigset );
	sigaddset( &sigset, SIGALRM );
	sigaddset( &sigset, SIGINT );
	pthread_sigmask ( SIG_BLOCK, &sigset, NULL );

	// now create the starter thread

	if( pthread_create( &tid, NULL, starter, NULL ) != 0 )
	{
		cerr << "ERR: cannot create the starter thread\n";
		exit( -1 );
	}

	// set the timer that will send SIGALRM signal periodically

	gettimeofday( &lastStat, NULL );
	
	sec = interval;
	itimer.it_interval.tv_sec = sec;
	itimer.it_interval.tv_usec = 0;
	itimer.it_value.tv_sec = sec;
	itimer.it_value.tv_usec = 0;
	if( setitimer( ITIMER_REAL, &itimer, NULL ) )
	{
		perror( "ERR: cannot set the timer" );
		exit( -1 );
	}

	// wait for SIGALRM andn SIGINT in infinite while cycle

	while( -1 )
	{
		sigwait( &sigset, &n );
		ostream.str( "" );
		if( n == SIGINT )  // quit
		{
			if( writestat ) statfile.close();
			exit( 0 );
		}
		
		gettimeofday( &now, NULL );   // the starting time of the new statistical interval
		timediff = timeDiff( now, lastStat );  // time from the previous stat interval
		lastStat = now;
		rate = ( float )g_bytes * ( 8000.0 / ( float )timediff );
		g_bytes = 0;
		
		// print the statistics
		
		for( i = 0; i < files; i++ )
		{
			mystat = g_downloads + i;
			pthread_mutex_lock( &( mystat->mutex ) );
			fsize = mystat->size;
			x = mystat->num;
			time = mystat->time;
			mystat->num = 0;
			mystat->time.tv_sec = 0;
			mystat->time.tv_usec = 0;
			pthread_mutex_unlock( &( mystat->mutex ) );
			if( i == 0 )  // incomplete downloads
			{
				ostream << setw( 10 ) << "Incomplete" << setw( 8 ) << x << endl;
			}
			else
			{
				ostream << setw( 10 ) << fsize << setw( 8 ) << x \
				<< setw( 8 ) << setprecision( 2 ) << setiosflags( ios::fixed );
				if( x != 0 )
				{
					avgtime = time.tv_sec * 1000 + ( float )time.tv_usec/( float )1000;
					avgtime = ( float )avgtime / ( float )x;
					if( avgtime < 1000 ) ostream << avgtime << " m";
					else ostream << avgtime/1000 << " s";
				}
				else ostream << "-";
				ostream << endl;
			}
		}
		if( rate < 1000 ) ostream << setprecision( 2 ) << rate << " k";
		else ostream << setprecision( 2 ) << rate/1000 << " M";
		ostream << endl;
		ostream << setw( 28 ) << setfill( '-' ) << "-" << setfill( ' ' ) << endl << ends;
		cout << ostream.str();
		if( writestat ) statfile << ostream.str();
	}
}

// the client thread

void *receiver( void *arg )
{
	unsigned char c;
	char mybuf[10];
	int n, sock, rnum;
	int incomp;  // indicates incomplete download
	unsigned long fsize, bytes, *ulp;
	struct sockaddr_in remote;
	struct timeval start, end;
	Statistics *mystat;

	remote.sin_family = PF_INET;
	remote.sin_port = htons( g_port );
	remote.sin_addr.s_addr = g_ip;
	sock = socket( PF_INET, SOCK_STREAM, 0 );
	if( sock == -1 )
	{
		perror( "ERR: cannot open socket" );
		pthread_exit( NULL );
	}
	n = DEF_WINDOW;
	if( setsockopt( sock, SOL_SOCKET, SO_RCVBUF, &n, sizeof( n ) ) )
	{
		perror( "ERR: cannot set SO_RCVBUF socket option" );
	}
	c = g_tos;
	if( setsockopt( sock, SOL_IP, IP_TOS, ( void * )&c, sizeof( c ) ) )
	{
		perror( "ERR: cannot set TOS for the socket" );
		exit( -1 );
	}
	gettimeofday( &start, NULL );  // the starting time of this download
	if( connect( sock, ( struct sockaddr * )&remote, sizeof( remote ) ) )
	{
		perror( "ERR: cannot connect to server" );
		pthread_exit( NULL );
	}
		
	// now generate a random number between 0 and g_arrsize

	rnum = ( int )( ( float )g_arrsize*rand()/( ( float )RAND_MAX ) );
	mystat = g_downloads + g_distArray[rnum];
	fsize = mystat->size;  // take the file size
	ulp = ( unsigned long * )mybuf;
	*ulp = htonl( fsize );  // put it into the sending buffer
	incomp = 0;
	if( send( sock, mybuf, sizeof( mybuf ), 0 ) == -1 )
	{
		incomp = -1;  // send error -> incomplete download
	}
	else
	{
		bytes = 0;
		while( -1 )
		{
			n = recv( sock, g_buf, sizeof( g_buf ), 0 );
			if( n < 1 )  // the server closed the connection
			{
				break;
			}
			bytes += n;
			g_bytes += n;
		}
		if( bytes < fsize )
		{
			incomp = -1;  // incomplete download
		}
	}
	if( incomp )
	{
		pthread_mutex_lock( &( g_downloads->mutex ) );
		( g_downloads->num )++;
		pthread_mutex_unlock( &( g_downloads->mutex ) );
	}
	else
	{
		gettimeofday( &end, NULL );  // the end of the download
		end -= start;  // the download time
		pthread_mutex_lock( &( mystat->mutex ) );

		// add the download to the stats

		mystat->time += end;
		( mystat->num )++;
		pthread_mutex_unlock( &( mystat->mutex ) );
	}
	close( sock );
	pthread_mutex_lock( &g_thread_mutex );
	g_threads--;  // decrease the thread counter
	pthread_mutex_unlock( &g_thread_mutex );
	return NULL;
}

// this thread starts the client threads

void *starter( void *arg )
{
	long diff;
	pthread_t tid;
	pthread_attr_t attr;
	Poisson poisson;
	struct timeval next, now;
	int go;
	
	poisson.avgint = g_waiting * 1000;// sets the mean value for the distribution (in us)
	pthread_attr_init( &attr );
	
	// we have to set the thread to detached, otherwise the resources of the
	// finished threads quickly fills the available kernel space
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
	gettimeofday( &next, 0 );
	while( -1 )
	{
		// now sleep for exponential time, and start a new client thread

		poisson.nextState();
		diff = ( long )poisson.getDifference();
		next += diff;
		gettimeofday( &now, 0 );
		diff = timeDiff( next, now );
		if( diff > 0 )	usleep( diff );
		
		// check the number of running clients
		
		pthread_mutex_lock( &g_thread_mutex );
		if( g_threads < g_max )
		{
			g_threads++;
			go = -1;  // we can go
		}
		else go = 0;
		pthread_mutex_unlock( &g_thread_mutex );
		if( go )
		{
			if( pthread_create( &tid, &attr, receiver, NULL ) != 0 )
			{
				cerr << "ERR: cannot create a client thread\n";
				exit( -1 );
			}
		}
	}
}

void usage()
{
	cerr << "Command line arguments:\n";
	cerr << "-ip server\n";
	cerr << "[-port port] : the default is " << DEF_SPORT << "\n";
	cerr << "[-max maxclients]\n";
	cerr << "[-file distfile]\n";
	cerr << "[-wait waittime] : in ms, the default is " << DEF_WAIT << "\n";
	cerr << "[-prec precedence]\n";
	cerr << "[-tos tos]\n";
	cerr << "[-int statinterval] : in ms, the default is 1000\n";
	cerr << "[-stat statfile] : to save the statistics\n";
	exit( -1 );
}
