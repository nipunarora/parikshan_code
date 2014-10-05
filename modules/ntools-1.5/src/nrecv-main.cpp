/*************************************************************************

 nrecv-main.cpp
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
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <exception>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netpacket/packet.h>
#include <linux/if_ether.h>

#include "nrecv.h"
#include "defs.h"
#include "range.h"
#include "utils.h"
#include "statistics.h"
#include "linkedlist.h"

using namespace std;

void usage();

int main( int argc, char **argv )
{
	ifstream scriptf;
	ostringstream ostream;
	ofstream statfile;
	int x;
	struct RStream *mystream;
	struct in_addr inaddr;
	unsigned long delay, jitter;
	int par_int, par_p1, par_p2, par_p3, par_level, par_id;
	
	cerr << "nrecv " << VERSION;

	try
	{
		NRecv nrecv;
		
		par_int = par_p1 = par_p2 = par_p3 = par_level = par_id = 0;
		delay = jitter = 0;
		
		// process the command line arguments
		if( ( argc > 1 ) && ( !strcmp( "-f", argv[1] ) ) )   // process the script file
		{
			if( argc != 3 ) usage();
			scriptf.open( argv[2], ios::in );
			if( !scriptf )
			{
				throw rerr + "cannot open " + argv[2];
			}
			// now read the config file
			cerr << "Reading config...\n";
			if( nrecv.processConfig( scriptf ) )
			{
				exit( -1 );
			}
			if( nrecv.streams.size() == 0 )
			{
				throw rerr + "no streams";
			}
			cerr << "Done\n";
		}
		else  // no script -> process the command line arguments
		{
			if( argc < 3 ) usage();
			mystream = new RStream;
			strcpy( mystream->id, "stream_0" );
			mystream->par_id = -1;
			mystream->ifname[IFNAMSIZ-1] = 0;
			while( argc > 1 )
			{
				if( !strcmp( "-delay", argv[1] ) )
				{
					mystream->measureDelay = -1;
					nrecv.measure_delay = -1;
					argc--, argv++;
				}
				else if( !strcmp( "-jitter", argv[1] ) )
				{
					mystream->measureJitter = -1;
					nrecv.measure_jitter = -1;
					argc--, argv++;
				}
				else if( !strcmp( "-timestamp", argv[1] ) )
				{
					nrecv.timestamp = -1;
					argc--, argv++;
				}
				else if( !strcmp( "-percentile", argv[1] ) )
				{
					mystream->measurePercentile = -1;
					nrecv.measure_percentile = -1;
					argc--, argv++;
				}
				else if( !strcmp( "-p1", argv[1] ) )
				{
					if( par_p1 ) throw rerr +"p1 parameter is duplicated";
					if( argc == 2 ) usage();
					nrecv.p1 = atof( argv[2] );
					if( nrecv.p1 <= 0 ) throw rerr +"invalid percentile: " + argv[2];
					par_p1 = -1;
					argc -= 2, argv += 2;
				}
				else if( !strcmp( "-p2", argv[1] ) )
				{
					if( par_p2 ) throw rerr +"p2 parameter is duplicated";
					if( argc == 2 ) usage();
					nrecv.p2 = atof( argv[2] );
					if( nrecv.p2 <= 0 ) throw rerr +"invalid percentile: " + argv[2];
					par_p2 = -1;
					argc -= 2, argv += 2;
				}
				else if( !strcmp( "-p3", argv[1] ) )
				{
					if( par_p3 ) throw rerr +"p3 parameter is duplicated";
					if( argc == 2 ) usage();
					nrecv.p3 = atof( argv[2] );
					if( nrecv.p3 <= 0 ) throw rerr +"invalid percentile: " + argv[2];
					par_p3 = -1;
					argc -= 2, argv += 2;
				}
				else if( !strcmp( "-int", argv[1] ) )
				{
					if( par_int ) throw rerr +"int parameter is duplicated";
					if( argc == 2 ) usage();
					nrecv.interval.tv_sec = readTime( argv[2] ) / 1000000;
					if( nrecv.interval.tv_sec < 1 or nrecv.interval.tv_sec > 1800 ) throw rerr +"invalid interval: " + argv[2] + ", must be between 1 and 1800";
					par_int = -1;
					argc -= 2, argv += 2;
				}
				else if( !strcmp( "-log", argv[1] ) )
				{
					if( nrecv.log ) throw rerr +"log parameter is duplicated";
					if( argc == 2 ) usage();
					nrecv.logfile.open( argv[2], ios::out );
					if( !nrecv.logfile ) throw rerr +"cannot open logfile: " + argv[2];
					nrecv.initLogfile();
					nrecv.log = -1;
					argc -= 2, argv += 2;
				}
				else if( !strcmp( "-statfile", argv[1] ) )
				{
					if( nrecv.writestat ) throw rerr +"statfile parameter is duplicated";
					if( argc == 2 ) usage();
					nrecv.statfile.open( argv[2], ios::out );
					if( !nrecv.statfile ) throw rerr +"cannot open statfile: " + argv[2];
					nrecv.writestat = -1;
					argc -= 2, argv += 2;
				}
				else if( !strcmp( "-if", argv[1] ) )
				{
					if( mystream->par_if ) throw rerr +"if parameter is duplicated";
					if( argc == 2 ) usage();
					strncpy( mystream->ifname, argv[2], IFNAMSIZ );
					mystream->ifname[IFNAMSIZ-1] = 0;
					mystream->par_if = -1;
					argc -= 2; argv += 2;
				}
				else if( !strcmp( "-srcip", argv[1] ) )
				{
					if( mystream->par_srcip ) throw rerr +"srcip parameter is duplicated";
					if( argc == 2 ) usage();
					if( inet_aton( argv[2], &inaddr ) == 0 ) throw rerr +"invalid srcip: " + argv[2];
					mystream->srcIp = inaddr.s_addr;
					mystream->par_srcip = -1;
					argc -= 2; argv += 2;
				}
				else if( !strcmp( "-dstip", argv[1] ) )
				{
					if( mystream->par_dstip ) throw rerr +"dstip parameter is duplicated";
					if( argc == 2 ) usage();
					if( inet_aton( argv[2], &inaddr ) == 0 ) throw rerr +"invalid dstip: " + argv[2];
					mystream->dstIp = inaddr.s_addr;
					mystream->par_dstip = -1;
					argc -= 2; argv += 2;
				}
				else if( !strcmp( "-proto", argv[1] ) )
				{
					if( mystream->par_proto ) throw rerr +"proto parameter is duplicated";
					if( argc == 2 ) usage();
					if( !strcmp( "tcp", argv[2] ) ) mystream->proto = IPPROTO_TCP;
					else if( !strcmp( "udp", argv[2] ) )
					{
						mystream->proto = IPPROTO_UDP;
					}
					else throw rerr +"invalid protocol: " + argv[2];
					mystream->par_proto = -1;
					argc -= 2; argv += 2;
				}
				else if( !strcmp( "-srcport", argv[1] ) )
				{
					if( mystream->par_srcport ) throw rerr +"srcport parameter is duplicated";
					if( argc == 2 ) usage();
					x = atoi( argv[2] );
					if( x < 0 || x > 65535 ) throw rerr +"invalid srcport: " + argv[2];
					mystream->srcPort = x;
					mystream->par_srcport = -1;
					argc -= 2; argv += 2;
				}
				else if( !strcmp( "-dstport", argv[1] ) )
				{
					if( mystream->par_dstport ) throw rerr +"dstport parameter is duplicated";
					if( argc == 2 ) usage();
					x = atoi( argv[2] );
					if( x < 0 || x > 65535 ) throw rerr +"invalid dstport: " + argv[2];
					mystream->dstPort = x;
					mystream->par_dstport = -1;
					argc -= 2; argv += 2;
				}
				else if( !strcmp( "-tos", argv[1] ) )
				{
					if( mystream->par_tos || mystream->par_prec ) throw rerr +"tos/prec parameter is duplicated";
					if( argc == 2 ) usage();
					x = atoi( argv[2] );
					if( x < 0 || x > 255 ) throw rerr +"invalid tos: " + argv[2];
					mystream->tos = x;
					mystream->par_tos = -1;
					argc -= 2; argv += 2;
				}
				else if( !strcmp( "-prec", argv[1] ) )
				{
					if( mystream->par_tos || mystream->par_prec ) throw rerr +"tos/prec parameter is duplicated";
					if( argc == 2 ) usage();
					x = atoi( argv[2] );
					if( x < 0 || x > 7 ) throw rerr +"invalid prec: " + argv[2];
					mystream->prec = x;
					mystream->par_prec = -1;
					argc -= 2; argv += 2;
				}
				else if( !strcmp( "-mcast", argv[1] ) )
				{
					mystream->mcast = -1;
					argc--, argv++;
				}
				else throw rerr + "invalid option: " + argv[1];
			}
			
			// check parameters
			nrecv.initStream( mystream );
			nrecv.streams.last();
			nrecv.streams.insertAfter( mystream );
		}  // end of command line processing
		
		nrecv.start();
	}
	catch( string e )
	{
		cerr << e << endl;
		return -1;
	}
}

void usage()
{
	cerr << "Command line arguments:\n";
	cerr << "-proto udp|tcp\n";
	cerr << "-if interface : for UDP streams only]\n";
	cerr << "[-srcip srcip] : for UDP streams only\n";
	cerr << "[-dstip dstip] : for UDP streams only\n";
	cerr << "[-srcport srcport] : for UDP streams only\n";
	cerr << "[-dstport dstport]\n";
	cerr << "[-prec precedence]\n";
	cerr << "[-tos tos]\n";
	cerr << "[-delay] : to measure delay\n";
	cerr << "[-jitter] : measure jitter\n";
	cerr << "[-percentile] : measure percentile\n";
	cerr << "[-p1 percentile1] : specify the first percentile\n";
	cerr << "[-p2 percentile1] : specify the second percentile\n";
	cerr << "[-p3 percentile1] : specify the third percentile\n";
	cerr << "[-int statinterval] : '2' - 2 sec, '100m' - 100 ms, the default is : 1 second\n";
	cerr << "[-timestamp : to timestamp each statistics line\n";
	cerr << "[-stat statfile] : to save the statistics\n";
	cerr << "[-log logfile] : to log every packets\n";
	cerr << "-----or----\n-f file\n";
	exit( -1 );
}
