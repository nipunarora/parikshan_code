/*************************************************************************

 ngen-main.cpp
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include <exception>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>

#include "defs.h"
#include "utils.h"
#include "distrib.h"
#include "linkedlist.h"
#include "ngen.h"

using namespace std;

void usage();

int main( int argc, char **argv )
{
	ifstream scriptf;
	int x;
	struct in_addr inaddr;
	GStream *mystream;
	
	cerr << "ngen " << VERSION;
	
	try
	{
		NGen ngen;
	
		// process the command line arguments
	
		if( ( argc > 1 ) && ( !strcmp( "-f", argv[1] ) ) )   // process the script file
		{
			if( argc != 3 ) usage();
			scriptf.open( argv[2], ios::in );
			if( !scriptf )
			{
				throw gerr + "cannot open " + argv[2];
			}
			// now read the config file
			cerr << "Reading config...\n";
			if( ngen.processConfig( scriptf ) )
			{
				exit( -1 );
			}
			if( ngen.streams.size() == 0 )
			{
				throw gerr + "no streams";
			}
			cerr << "Done\n";
		}
		else  // no script -> process the command line arguments
		{
			if( argc < 6 || argc%2 != 1 ) usage();
			mystream = new GStream;
			mystream->vlan = -1;
			Fix *fixp = new Fix;
			fixp->size = 1518;  // default frame size
			
			// read the arguments
			while( argc != 1 )
			{
				if( !strcmp( "-srcip", argv[1] ) )
				{
					if( inet_aton( argv[2], &inaddr ) == 0 ) throw gerr + "invalid srcip: " + argv[2];
					mystream->srcIp = inaddr.s_addr;
					mystream->par_srcip = -1;
				}
				else if( !strcmp( "-dstip", argv[1] ) )
				{
					if( inet_aton( argv[2], &inaddr ) == 0 ) throw gerr + "invalid dstip: " + argv[2];
					mystream->dstIp = inaddr.s_addr;
					mystream->par_dstip = -1;
				}
				else if( !strcmp( "-if", argv[1] ) )
				{
					strncpy( mystream->ifname, argv[2], IFNAMSIZ );
					mystream->ifname[IFNAMSIZ-1] = 0;
					mystream->par_if = -1;
				}
				else if( !strcmp( "-proto", argv[1] ) )
				{
					if( !strcmp( "tcp", argv[2] ) ) mystream->proto = IPPROTO_TCP;
					else if( !strcmp( "udp", argv[2] ) )
					{
						mystream->proto = IPPROTO_UDP;
					}
					else
					{
						throw gerr + "invalid protocol: " + argv[2];
					}
					mystream->par_proto = -1;
				}
				else if( !strcmp( "-srcport", argv[1] ) )
				{
					x = atoi( argv[2] );
					if( x < 0 || x > 65535 ) throw gerr + "invalid srcport: " + argv[2];
					mystream->srcPort = x;
					mystream->par_srcport = -1;
				}
				else if( !strcmp( "-dstport", argv[1] ) )
				{
					x = atoi( argv[2] );
					if( x < 0 || x > 65535 ) throw gerr + "invalid dstport: " + argv[2];
					mystream->dstPort = x;
					mystream->par_dstport = -1;
				}
				else if( !strcmp( "-tos", argv[1] ) )
				{
					x = atoi( argv[2] );
					if( x < 0 || x > 255 ) throw gerr + "invalid tos: " + argv[2];
					mystream->tos = x;
					mystream->par_tos = -1;
				}
				else if( !strcmp( "-prec", argv[1] ) )
				{
					x = atoi( argv[2] );
					if( x < 0 || x > 7 )	throw gerr + "invalid prec: " + argv[2];
					mystream->tos = x << 5;
					mystream->par_tos = -1;
				}
				else if( !strcmp( "-rate", argv[1] ) )
				{
					if( !strcmp( argv[2], "max" ) ) x = -1;
					else
					{
						x = readRate( argv[2] );
						if( x < MIN_RATE || x > MAX_RATE ) throw gerr + "invalid rate: " + argv[2] + ", must be between 10k and 1000M";
					}
					fixp->rate = x;
					mystream->dist = ( Distrib * )fixp;
				}
				else if( !strcmp( "-size", argv[1] ) )
				{
					x = atoi( argv[2] );
					if( x < 64 || x > 9018 ) throw gerr + "invalid size: " + argv[2] + ", must be between 64..9018: ";
					fixp->size = x;
				}
				else
				{
					throw gerr + "invalid option: " + argv[1];
				}
				argc -= 2;
				argv += 2;
			}
			// check parameters
			ngen.initStream( mystream );
			ngen.streams.last();
			ngen.streams.insertAfter( mystream );
		}  // end of command line processing
		
		ngen.start();
	}
	catch( string e )
	{
		cerr << e << endl;
	}
}

void usage()
{
	cerr << "Command line arguments:\n";
	cerr << "-proto udp|tcp\n";
	cerr << "-if : interface\n";
	cerr << "-dstip dstip\n";
	cerr << "-rate rate : ( like 500k or 10M, optional for TCP )\n";
	cerr << "[-size size] : in bytes\n";
	cerr << "[-srcip srcip]\n";
	cerr << "[-srcport srcport] : the default is " << DEF_SPORT << "\n";
	cerr << "[-dstport dstport]\n";
	cerr << "[-tos tos]\n";
	cerr << "[-prec precedence]\n";
	cerr << "[-aw] : active waiting\n";
	cerr << "-----or----\n-f file\n";
	exit( -1 );
}
