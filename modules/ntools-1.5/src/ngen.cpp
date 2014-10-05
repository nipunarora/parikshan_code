/*************************************************************************

 ngen.cpp
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/

#include <cstdlib>
#include <stdio.h>
#include <errno.h>
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
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>

#include "defs.h"
#include "range.h"
#include "utils.h"
#include "distrib.h"
#include "linkedlist.h"
#include "ngen.h"

using namespace std;

// global big TCP buffer
char tcpbuf[65536];
int tcpbuflen = 65536;

// thread argument to the tcp sender
struct TcpArg
{
	NGen *ngen;
	GStream *mystream;
};

// tcp sender thread
void *tcpSenderThread( void *arg )
{
	NGen *ngen;
	TcpArg *tcparg;
	GStream *mystream;
	
	tcparg = ( TcpArg * )arg;
	ngen = tcparg->ngen;
	mystream = tcparg->mystream;
	ngen->tcpSender( mystream );
	return 0;
}

// udp sender thread
void *udpSenderThread( void *arg )
{
	NGen *ngen;
	
	ngen = ( NGen * )arg;
	ngen->udpSender();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
GStream::GStream()
{
	memset( ( void * )this, 0, sizeof( GStream ) );
}

GStream::~GStream()
{
	if( srcIpRange ) delete srcIpRange;
	if( dstIpRange ) delete dstIpRange;
	if( srcPortRange ) delete srcPortRange;
	if( dstPortRange ) delete dstPortRange;
	if( dist ) delete dist;
};


///////////////////////////////////////////////////////////////////////////////
NGen::NGen()
{
	pthread_t tid;
	
	run = 0;
	command_mode = 0;
	udpstreams = 0;
	udprun = 0;
	rt = checkRT();
	
	if( getuid() )
	{
		error( "ngen must run with root user id" );
		exit( -1 );
	}
	
	tid = pthread_self();
	pthread_cond_init( &cond_udprun, 0 );
	pthread_mutex_init( &mutex_udprun, 0 );
	streams.threadSafe();

	// create the udp sender thread
	if( pthread_create( &tid, 0, udpSenderThread, ( void * )this ) != 0 )
	{
		throw gerr + "cannot create the udp sender thread";
		if( !command_mode ) exit( -1 );
	}
}


///////////////////////////////////////////////////////////////////////////////
void NGen::start()
{
	GStream *mystream;
	
	if( run ) return;
	if( !command_mode )
	{
		if( !streams.size() )
		{
			error( "no active streams" );
			exit( -1 );
		}
	}
	streams.begin();
	while( ( mystream = streams.next() ) )
	{
		startStream( mystream );
	}
	run = -1;
	if( udpstreams )
	{
		pthread_mutex_lock( &mutex_udprun );
		udprun = -1;
		pthread_cond_broadcast( &cond_udprun );
		pthread_mutex_unlock( &mutex_udprun );
	}
	if( !command_mode ) pause();
}


///////////////////////////////////////////////////////////////////////////////
void NGen::stop()
{
	GStream *mystream;
	
	if( !run ) return;
	run = 0;
	udprun = 0;
	streams.begin();
	while( ( mystream = streams.next() ) )
	{
		if( mystream->proto == IPPROTO_TCP )
		{
			pthread_cancel( mystream->tid );
			pthread_join( mystream->tid, 0 );
			close( mystream->sock );
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// The udp sender, it handles all udp traffic
void NGen::udpSender()
{
	char buf[9100];
	char *vlanbuf;
	char *ipbuf;
	struct iphdr *iph;
	struct udphdr * udph;
	struct vlanhdr *vlanh;
	unsigned long *ubuf;
	unsigned long wait;
	int size, x, ret;
	int rawsock, packetsock, type;
	struct timeval now, tmptime, next_time;
	struct sockaddr_in udpremote;
	int addrll_len, addrin_len;
	GStream *mystream, *next_stream, sending_stream;
	struct sched_param sch_param;
	
	// init
	try
	{
		type = 0;
		vlanbuf = buf;
		vlanh = ( struct vlanhdr * )buf;
		ipbuf = buf + 4;
		iph = ( struct iphdr * )ipbuf;
		udph = ( struct udphdr * )( ipbuf + 20 );
		addrll_len = sizeof( struct sockaddr_ll );
		addrin_len = sizeof( struct sockaddr_in );
		packetsock = rawsock = 0;
		next_stream = 0;
		
		// set the scheduling parameters
		if( rt )
		{
			sch_param.sched_priority = 99;
			if(  pthread_setschedparam( pthread_self(), SCHED_FIFO, &sch_param ) )
			{
				error( "cannot set priority scheduling: ", errno );
				pthread_exit( NULL );
			}
		}
		
		// build the headers
		vlanh->myunion.id = 0;
		vlanh->myunion.mystruct.cfi = 0;
		vlanh->type = htons( ETH_P_IP );
	
		iph->version = 4;
		iph->ihl = 5;
		iph->id = 0;
		iph->frag_off = htonl( 0x4000 );  // don't fragment
		iph->ttl = 64;
		iph->protocol = IPPROTO_UDP;
		
		udph->check = 0;
		
		// create the packet socket
		packetsock = socket( PF_PACKET, SOCK_DGRAM, htons( ETH_P_IP ) );
		if( packetsock == -1 )
		{
			error( "Cannot create a packet socket: ", errno );
			pthread_exit( NULL );
		}
	
		// open the raw socket for normal udp streams
		rawsock = socket( PF_INET, SOCK_RAW, IPPROTO_RAW );
		if( rawsock == -1 )
		{
			error( "Cannot create the raw socket: ", errno );
			pthread_exit( NULL );
		}
		udpremote.sin_family = PF_INET;
		udpremote.sin_port = htons( IPPROTO_UDP );
		
		// traffic generation
		while( -1 )
		{
			if( !udprun )
			{
				pthread_mutex_lock( &mutex_udprun );
				while( !run )
				{
					pthread_cond_wait( &cond_udprun, &mutex_udprun );
				}
				pthread_mutex_unlock( &mutex_udprun );
			}
			x = 0;
			streams.wrlock();
			streams.begin();
			while( ( mystream = streams.next() ) )
			{
				if( mystream->proto != IPPROTO_UDP ) continue;
				tmptime = mystream->dist->getSendTime();
				if( x == 0 )
				{
					next_time = tmptime;
					next_stream = mystream;
					x = -1;
				}
				else
				{
					if( tmptime < next_time )
					{
						next_time = tmptime;
						next_stream = mystream;
					}
				}
			}
			if( !x )  // no active udp streams
			{
				streams.unlock();
				continue;
			}
			size = next_stream->dist->getSize();
			size -= 18;
			if( size > 9018 ) size = 9018;
			if( next_stream->vlan != -1 ) size -= 4;
			if( size < 46 ) size = 46;
			if( next_stream->srcIpRange != 0 ) next_stream->srcIp = next_stream->srcIpRange->getNextIp();
			if( next_stream->dstIpRange != 0 ) next_stream->dstIp = next_stream->dstIpRange->getNextIp();
			if( next_stream->srcPortRange != 0 ) next_stream->srcPort = next_stream->srcPortRange->getNextPort();
			if( next_stream->dstPortRange != 0 ) next_stream->dstPort = next_stream->dstPortRange->getNextPort();
			
			iph->tot_len = htons( size );
			iph->saddr = next_stream->srcIp;
			iph->daddr = next_stream->dstIp;
			iph->tos = next_stream->tos;
			
			udph->len = htons( size - 20 );
			udph->source = htons( next_stream->srcPort );
			udph->dest = htons( next_stream->dstPort );
			memcpy( ipbuf+28, "ntools", 6 );  // place an identifier
			ubuf = ( unsigned long * )( ipbuf+34 );   // skip the ip and udp header
			ubuf[0] = htonl( next_stream->seqNum );  // place the sequence number
			next_stream->seqNum++;
			next_stream->dist->nextState();
			// WARNING: The stream might be deleted at the time of sending, therefore we make a copy of it.
			// Be sure not to use any pointers after the stream list is unlocked!
			sending_stream = *next_stream;
			
			streams.unlock();
			
			gettimeofday( &now, NULL );
			// wait for sending time
			if( next_time > now )
			{
				if( rt )
				{
					wait = timeDiff( next_time, now );
					if( wait > WAITING_TIMELIMIT )
					{
						usleep( wait );
					}
				}
				else
				{
					while( next_time > now )
					{
						gettimeofday( &now, NULL );
					}
				}
			}
			gettimeofday( &now, NULL );
			ubuf[1] = htonl( now.tv_sec );  // place timestamp
			ubuf[2] = htonl( now.tv_usec );
			if( sending_stream.par_dstmac )
			{
				if( sending_stream.vlan == -1 )
				{
					sending_stream.addrll.sll_protocol = htons( ETH_P_IP );
					computeChecksum( ( struct iphdr * )ipbuf );
					ret = sendto( packetsock, ipbuf, size, 0, ( struct sockaddr * )&( sending_stream.addrll ), addrll_len );
				}
				else
				{
					sending_stream.addrll.sll_protocol = htons( 0x8100 );  // VLAN
					vlanh->myunion.mystruct.prio = ( unsigned short )sending_stream.pbits;
					vlanh->myunion.id |= htons( ( unsigned short )sending_stream.vlan );
					computeChecksum( ( struct iphdr * )ipbuf );
					ret = sendto( packetsock, vlanbuf, size+4, 0, ( struct sockaddr * )&( sending_stream.addrll ), addrll_len );
				}
			}
			else
			{
				if( setsockopt( rawsock, SOL_SOCKET, SO_BINDTODEVICE, sending_stream.ifname, sizeof( sending_stream.ifname ) ) )
				{
					perror( "Cannot bind" );
				}
				udpremote.sin_addr.s_addr = sending_stream.dstIp;
				ret = sendto( rawsock, ipbuf, size, 0, ( struct sockaddr * )&udpremote, addrin_len );
			}
			if( ret == -1 )
			{
				error( "Send error: ", errno );
				pthread_exit( NULL );
			}
		}
	}
	catch( string e )
	{
		error( e );
		pthread_exit( 0 );
	}
}


///////////////////////////////////////////////////////////////////////////////
// This is called upon an error. The message is either printed on stdout,
// or in command mode inserted into the results linked list.
void NGen::error( string err, int errnum )
{
	char buf[256];
	string *mystring;
	
	if( command_mode )
	{
		mystring = new string;
		*mystring = "ERR ngen: ";
		*mystring += err;
		if( errnum )
		{
			strerror_r( errnum, buf, 256 );
			*mystring += buf;
		}
		*mystring += "\n";
		results->add( mystring );
	}
	else
	{
		cerr << "ERR: " << err << endl;
		exit( -1 );
	}
}


///////////////////////////////////////////////////////////////////////////////
// The tcp sender it handles one normal tcp connection
void NGen::tcpSender( GStream *mystream )
{
	char *buf;
	int x, size;
	unsigned char c;
	unsigned long wait;
	struct timeval now, next_time;
	struct sockaddr_in remote, local;
	ostringstream stream;

	try
	{
		mystream->tid = pthread_self();
		buf = tcpbuf;
		
		while( -1 )
		{
			mystream->sock = socket( PF_INET, SOCK_STREAM, 0 );
			if( mystream->sock == -1 )
			{
				error( "Cannot open TCP mystream->socket: ", errno );
				pthread_exit( NULL );
			}
			x = -1;
			if( setsockopt( mystream->sock, SOL_SOCKET, SO_REUSEADDR, &x, sizeof( x ) ) )
			{
				error( "Cannot set SO_REUSEADDR mystream->socket option: ", errno );
				pthread_exit( NULL );
			}
			c = mystream->tos;
			if( setsockopt( mystream->sock, SOL_IP, IP_TOS, ( void * )&c, sizeof( c ) ) )
			{
				error( "Cannot set tos for TCP mystream->socket", errno );
				pthread_exit( NULL );
			}
			x = -1;
			local.sin_port = htons( mystream->srcPort );
			local.sin_addr.s_addr = htonl(INADDR_ANY);
			//local.sin_addr.s_addr = getIfAddr( mystream->ifname );
			if( bind( mystream->sock, ( struct sockaddr * )&local, sizeof( local ) ) == -1 )
			{
				stream.str( "" );
				stream << "Cannot bind TCP mystream->socket to local port: ";
				stream << mystream->srcPort << ": " << errno;
				error( stream.str().c_str() );
				pthread_exit( NULL );
			}
			remote.sin_family = PF_INET;
			remote.sin_addr.s_addr = mystream->dstIp;
			remote.sin_port = htons( mystream->dstPort );
				
			while( connect( mystream->sock, ( struct sockaddr * )&remote, sizeof( remote ) ) )
			{
				sleep( 1 );  // cannot connect, try later
			}
			while( -1 )
			{
				if( mystream->dist )
				{
					gettimeofday( &now, NULL );
					next_time = mystream->dist->getSendTime();
					if( next_time > now )
					{
						if( rt )
						{
							wait = timeDiff( next_time, now );
							if( wait > WAITING_TIMELIMIT )
							{
								usleep( wait );
							}
						}
						else
						{
							while( next_time > now )
							{
								gettimeofday( &now, NULL );
							}
						}
					}
					size = mystream->dist->getSize();
				}
				else
				{
					size = tcpbuflen;
				}
				if( send( mystream->sock, buf, size, MSG_NOSIGNAL ) == -1 )
				{
					close( mystream->sock );  // session got broken
					break;  // break and try to recconnect
				}
				if( mystream->dist )
				{
					gettimeofday( &now, NULL );
					mystream->dist->nextState();  // calculate next time and size
					if( mystream->dist->getSendTime() < now )
					{
						mystream->dist->setSendTime( now );  // sending time of the last packet
					}
				}
			}
		}
	}
	catch( string e )
	{
		error( e );
		pthread_exit( 0 );
	}
}


///////////////////////////////////////////////////////////////////////////////
void NGen::initStream( GStream *mystream )
{
	int x, i;
	GStream *streamp;
	
	// check settings
	streams.begin();
	if( command_mode )
	{
		while( ( streamp = streams.next() ) )
		{
			if( !strcmp( streamp->id, mystream->id ) )
			{
				throw gerr + "stream with id " + mystream->id + " alreay exists";
			}
		}
	}
	if( !mystream->par_if )
	{
		throw gerr + "if parameters is missing";
	}
	if( !mystream->srcPort ) mystream->srcPort = DEF_SPORT;
	if( command_mode and !mystream->par_id ) throw gerr + "id parameter is missing";
	if( !mystream->par_dstip ) throw gerr + "dstip parameter is missing";
	if( !mystream->par_proto ) throw gerr + "proto parameter is missing";
	if( !mystream->par_dstport ) throw gerr + "dstport parameter is missing";
	if( mystream->proto == IPPROTO_TCP )
	{
		if( mystream->srcIpRange || mystream->dstIpRange || mystream->srcPortRange \
		|| mystream->dstPortRange ) throw gerr + "address and port ranges are not supported for TCP streams";
		if( mystream->par_srcip ) throw gerr + "srcip parameter is not supported for TCP streams";
		if( mystream->par_dstmac ) throw gerr + "dstmac parameter is not supported for TCP streams";
		if( mystream->par_vlan ) throw gerr + "vlan parameter is not supported for TCP streams";
		if( mystream->par_pbits ) throw gerr + "pbits parameter is not supported for TCP streams";
	}
	else
	{
		if( !mystream->par_vlan && mystream->par_pbits ) throw gerr + "pbits parameter must be specified together with the vlan parameter";
		if( !mystream->par_dstmac && mystream->par_vlan ) throw gerr + "vlan parameter must be specified together with the dstmac parameter";
	}
	if( !mystream->dist && ( mystream->proto != IPPROTO_TCP ) ) throw gerr + "distrib parameter is missing";
	if( mystream->dist )
	{
	  /*
		if( ( mystream->proto == IPPROTO_TCP ) and ( mystream->dist->type() != Distrib::FIX ) )
		{
			throw gerr + "TCP streams support only fix distributions";
		}
	  */
		if( mystream->dist->type() == Distrib::FIX )
		{
			if( !(( Fix * )mystream->dist )->rate ) throw gerr + "distrib fix: rate parameter is missing";
		}
		else if( mystream->dist->type() == Distrib::ONOFF )
		{
			if( (( OnOff * )mystream->dist )->rate1 == 0 ) throw gerr + "distrib onoff: rate1 parameter is missing";
			if( (( OnOff * )mystream->dist )->rate2 == 0 ) throw gerr + "distrib onoff: rate2 parameter is missing";
			if( (( OnOff * )mystream->dist )->time1 == 0 ) throw gerr + "distrib onoff: time1 parameter is missing";
			if( (( OnOff * )mystream->dist )->time2 == 0 ) throw gerr + "distrib onoff: time2 parameter is missing";
		}
		else if( mystream->dist->type() == Distrib::POISSON )
		{
			if( (( Poisson * )mystream->dist )->avgint == 0 ) throw gerr + "distrib poissoin: avgint parameter is missing";
		}
	}
	// initialize
	if( mystream->proto == IPPROTO_UDP )
	{
		if( !mystream->par_srcip )
		{
			mystream->srcIp = getIfAddr( mystream->ifname );
		}
		if( mystream->par_dstmac )
		{
			mystream->addrll.sll_family = AF_PACKET;
			try
			{
				mystream->addrll.sll_ifindex = getIfIndex( mystream->ifname );
			}
			catch( string e )
			{
				error( e, errno );
				pthread_exit( NULL );
			}
			mystream->addrll.sll_halen = 6;
			x = 0;
			for( i = 0; i < 6; i++ )
			{
				mystream->addrll.sll_addr[i] = hexConverter( mystream->dstMac[x] )*16 + hexConverter( mystream->dstMac[x+1] );
				x += 2;
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
void NGen::startStream( GStream *mystream )
{
	pthread_t tid;
	TcpArg *tcparg;
	
	if( mystream->dist ) mystream->dist->init();  // init the distribution class
	if( mystream->proto == IPPROTO_UDP )
	{
		udpstreams++;
		if( run and !udprun )
		{
			pthread_mutex_lock( &mutex_udprun );
			udprun = -1;
			pthread_cond_broadcast( &cond_udprun );
			pthread_mutex_unlock( &mutex_udprun );
		}
	}
	else
	{
		tcparg = new TcpArg;
		tcparg->ngen = this;
		tcparg->mystream = mystream;
		if( pthread_create( &tid, 0, tcpSenderThread, ( void * )tcparg ) != 0 )
		{
			throw gerr + "cannot create a TCP sender thread";
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Reads configuration from a stream.
int NGen::processConfig( istream &file )
{
	char buf[256];
	int buflen = 256;
	struct in_addr inaddr;
	int type = 0;
	unsigned int i, x;
	GStream *mystream;
	ostringstream ostream;
	
	while( nextWord( file, buf, 256 ) != -1 )  // while there is a keyword
	{
		if( !strcmp( "start", buf ) )
		{
			start();
			return 0;
		}
		else if( !strcmp( "stop", buf ) )
		{
			stop();
			return 0;
		}
		else if( !strcmp( "clear", buf ) )
		{
			stop();
			streams.wrlock();
			streams.clear();
			streams.unlock();
			return 0;
		}
		else if( !strcmp( "delete", buf ) )
		{
			if( nextWord( file, buf, buflen ) )
			{
				throw gerr + "stream id is expected after delete keyword";
			}
			streams.begin();
			while( ( mystream = streams.next() ) )
			{
				if( !strcmp( buf, mystream->id ) ) break;
			}
			if( streams.end() )
			{
				throw gerr + "stream " + buf + " does not exist";
			}
			if( mystream->proto == IPPROTO_TCP )
			{
				pthread_cancel( mystream->tid );
				pthread_join( mystream->tid, 0 );
				close( mystream->sock );
			}
			else
			{
				if( udpstreams == 1 )
				{
					udprun = 0;
				}
				udpstreams--;
			}
			streams.wrlock();
			streams.remove();
			streams.unlock();
		}
		else if( !strcmp( "add", buf ) )
		{
			if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
			if( strcmp( "{", buf ) ) throw gerr + "keyword { is expected";
			
			// set the default parameters
			
			mystream = new GStream;
			mystream->vlan = -1;
			mystream->srcPort = DEF_SPORT;
			
			// now read the parameters of the stream
			
			while( -1 )
			{
				if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
				if( !strcmp( "id", buf ) )
				{
					if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
					if( command_mode )
					{
						x = ID_SIZE;
					}
					else
					{
						x = ID_CLI_SIZE;
					}
					if( strlen( buf ) > x  )
					{
						ostream.str( "" );
						ostream << "stream id can be maximum " << x << " characters: " << buf;
						throw gerr + ostream.str();
					}
					strcpy( mystream->id, buf );
					mystream->par_id = -1;
				}
				else if( !strcmp( "if", buf ) )
				{
					if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
					strncpy( mystream->ifname, buf, IFNAMSIZ );
					mystream->ifname[IFNAMSIZ-1] = 0;
					mystream->par_if = -1;
				}
				else if( !strcmp( "srcip", buf ) )
				{
					if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
					if( !strcmp( "range", buf ) )
					{
						mystream->srcIpRange = new IpRange;
						if( mystream->srcIpRange == 0 ) throw gerr + "cannot allocate memory";
						readIpRange( *mystream->srcIpRange, file, buf, 256 );
					}
					else
					{
						if( inet_aton( buf, &inaddr ) == 0 ) throw gerr + "invalid srcip: " + buf;
						mystream->srcIp = inaddr.s_addr;
						mystream->srcIpRange = 0;
					}
					mystream->par_srcip = -1;
				}
				else if( !strcmp( "dstip", buf ) )
				{
					if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
					if( !strcmp( "range", buf ) )
					{
						mystream->dstIpRange = new IpRange;
						if( mystream->dstIpRange == 0 ) throw gerr + "cannot allocate memory";
						readIpRange( *mystream->dstIpRange, file, buf, 256 );
					}
					else
					{
						if( inet_aton( buf, &inaddr ) ==  0 ) throw gerr + "invalid dstip: " + buf;
						mystream->dstIp = inaddr.s_addr;
					}
					mystream->par_dstip = -1;
				}
				else if( !strcmp( "vlan", buf ) )
				{
					if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
					x = atoi( buf );
					if( x < 0 || x > 4095 ) throw gerr + "invalid vlan: " + buf;
					mystream->vlan = x;
					mystream->par_vlan = -1;
				}
				else if( !strcmp( "pbits", buf ) )
				{
					if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
					x = atoi( buf );
					if( x < 0 || x > 4095 ) throw gerr + "invalid pbits: " + buf;
					mystream->pbits = x;
					mystream->par_pbits = -1;
				}
				else if( !strcmp( "dstmac", buf ) )
				{
					if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
					for( i = 0, x = 0; i < sizeof( buf ); i ++ )
					{
						if( buf[i] != ':' )
						{
							mystream->dstMac[x] = buf[i];
							if( ( x == 12 ) and ( buf[i] != 0 ) ) throw gerr + " invalid dstmac";
							if( ( x != 12 ) and ( buf[i] == 0 ) ) throw gerr + " invalid dstmac";
							if( buf[i] == 0 ) break;
							x++;
						}
					}
					mystream->ifname[12] = 0;
					mystream->par_dstmac = -1;
				}
				else if( !strcmp( "outif", buf ) )
				{
					if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
					strncpy( mystream->ifname, buf, IFNAMSIZ );
					mystream->ifname[IFNAMSIZ-1] = 0;
					mystream->par_outif = -1;
				}
				else if( !strcmp( "proto", buf ) )
				{
					if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
					if( !strcmp( "tcp", buf ) ) mystream->proto = IPPROTO_TCP;
					else if( !strcmp( "udp", buf ) )
					{
						mystream->proto = IPPROTO_UDP;
					}
					else throw gerr + "invalid protocol: " + buf;
					mystream->par_proto = -1;
				}
				else if( !strcmp( "tos", buf ) )
				{
					if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
					x = atoi( buf );
					if( x < 0 || x > 255 ) throw gerr + "invalid tos: " + buf;
					mystream->tos = x;
					mystream->par_tos = -1;
				}
				else if( !strcmp( "prec", buf ) )
				{
					if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
					x = atoi( buf );
					if( x < 0 || x > 7 ) throw gerr +  "invalid prec: " + buf;
					x <<= 5;
					mystream->tos = x;
					mystream->par_tos = -1;
				}
				else if( !strcmp( "srcport", buf ) )
				{
					if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
					if( !strcmp( "range", buf ) )
					{
						mystream->srcPortRange = new PortRange;
						if( mystream->srcPortRange == 0 ) throw gerr + "cannot allocate memory";
						readPortRange( *mystream->srcPortRange, file, buf, 256 );
						mystream->srcPort = 1;
					}
					else
					{
						x = atoi( buf );
						if( x < 0 || x > 65535 ) throw gerr + "invalid srcport: " + buf;
						mystream->srcPort = x;
						mystream->srcPortRange = 0;
					}
					mystream->par_srcport = -1;
				}
				else if( !strcmp( "dstport", buf ) )
				{
					if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
					if( !strcmp( "range", buf ) )
					{
						mystream->dstPortRange = new PortRange;
						if( mystream->dstPortRange == 0 ) throw gerr + "cannot allocate memory";
						readPortRange( *mystream->dstPortRange, file, buf, 256 );
						mystream->dstPort = 1;
					}
					else
					{
						x = atoi( buf );
						if( x < 0 || x > 65535 ) throw gerr + "invalid dstport: " + buf;
						mystream->dstPort = x;
						mystream->dstPortRange = 0;
					}
					mystream->par_dstport = -1;
				}
				else if( !strcmp( "distrib", buf ) )
				{
					if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
					if( !strcmp( "fix", buf ) ) type = Distrib::FIX;  // fix distribution
					else if( !strcmp( "tcp", buf ) )  // tcp stream
					{
						mystream->dist = 0;
						continue;
					}
					else if( !strcmp( "poisson", buf ) ) type = Distrib::POISSON;  // poisson distribution
					else if( !strcmp( "on-off", buf ) ) type = Distrib::ONOFF;  // onoff distribution
					else throw gerr + "unknown distribution: " + buf;
					if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
					if( strcmp( "{", buf ) ) throw gerr + "keyword { is expected";
	
					// now read the distribution parameters
	
					if( type == Distrib::FIX )
					{
						Fix *fixp = new Fix;
						fixp->size = 1518;  // default frame size
						while( -1 )
						{
							if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
							if( !strcmp( "size", buf ) )
							{
								if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
								x = atoi( buf );
								if( x < 64 || x > 9018 ) throw gerr + "invalid size: " + buf;
								fixp->size = x;
							}
							else if( !strcmp( "rate", buf ) )
							{
								if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
								if( !strcmp( buf, "max" ) )
								{
									fixp->rate = -1;
								}
								else
								{
									x = readRate( buf );
									if( x < MIN_RATE || x > MAX_RATE ) throw gerr + "invalid rate: " + buf + ", must be between 10k and 1000M";
									fixp->rate = x;
								}
							}
							else if( !strcmp( "}", buf ) ) break;
							else throw gerr + "invalid keyword: " + buf;
						}
						mystream->dist = ( Distrib * )fixp;
					}
					else if( type == Distrib::POISSON )
					{
						Poisson *poissonp = new Poisson;
						poissonp->size = 1518;  // default packet size
						while( -1 )
						{
							if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
							if( !strcmp( "size", buf ) )
							{
								if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
								x = atoi( buf );
								if( x < 64 || x > 9018 ) throw gerr + "invalid size: " + buf;
								poissonp->size = x;
							}
							else if( !strcmp( "avgint", buf ) )
							{
								if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
								x = readTime( buf );
								if( x < 1 ) throw gerr + "invalid avgint: " + buf;
								poissonp->avgint = x;
							}
							else if( !strcmp( "}", buf ) ) break;
							else throw gerr + "invalid keyword: " + buf;
						}
						mystream->dist = ( Distrib * )poissonp;
					}
					else if( type == Distrib::ONOFF )
					{
						OnOff *onoffp = new OnOff;
						onoffp->size1 = 1518;
						onoffp->size2 = 1518;
						while( -1 )
						{
							if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
							if( !strcmp( "size1", buf ) )
							{
								if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
								x = atoi( buf );
								if( x < 64 || x > 9018 ) throw gerr + "invalid size1: " + buf;
								onoffp->size1 = x;
							}
							else if( !strcmp( "size2", buf ) )
							{
								if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
								x = atoi( buf );
								if( x < 64 || x > 9018 ) throw gerr + "invalid size2: " + buf;
								onoffp->size2 = x;
							}
							else if( !strcmp( "rate1", buf ) )
							{
								if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
								x = readRate( buf );
								if( x < MIN_RATE || x > MAX_RATE ) throw gerr + "invalid rate1: " + buf + ", must be between 10k and 1000M";
								onoffp->rate1 = x;
							}
							else if( !strcmp( "rate2", buf ) )
							{
								if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
								x = readRate( buf );
								if( x < MIN_RATE || x > MAX_RATE ) throw gerr + "invalid rate2: " + buf + ", must be between 10k and 1000M";
								onoffp->rate2 = x;
							}
							else if( !strcmp( "time1", buf ) )
							{
								if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
								x = readTime( buf );
								if( x < 1 ) throw gerr + "invalid time1: " + buf;
								onoffp->time1 = x;
							}
							else if( !strcmp( "time2", buf ) )
							{
								if( nextWord( file, buf, 256 ) ) throw gerr + "unexpected end of file";
								if( !strcmp( "infinite", buf ) ) onoffp->time2 = 0;
								else
								{
									x = readTime( buf );
									if( x < 1 ) throw gerr + "invalid time2: " + buf;
									onoffp->time2 = x;
								}
							}
							else if( !strcmp( "}", buf ) ) break;
							else throw gerr + "invalid keyword: " + buf;
						}
						mystream->dist = ( Distrib * )onoffp;
					}
				}
				else if( !strcmp( "}", buf ) ) break;
				else throw gerr + "invalid keyword: " + buf;
			}
			initStream( mystream );
			streams.wrlock();
			streams.last();
			streams.insertAfter( mystream );
			streams.unlock();
			if( run ) startStream( mystream );
		}
		else throw gerr + "invalid keyword: " + buf;
	}
	return 0;
}
