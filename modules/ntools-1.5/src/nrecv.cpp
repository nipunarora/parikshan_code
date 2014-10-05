/*************************************************************************

 nrecv.cpp
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/

#include <cstdlib>
#include <errno.h>
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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
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

// thread argument for the tcp recevier
struct TcpArg
{
	NRecv *nrecv;
	RStream *mystream;
};

// tcp receiver thread
void *tcpReceiverThread( void *arg )
{
	NRecv *nrecv;
	TcpArg *tcparg;
	RStream *mystream;
	
	tcparg = ( TcpArg * )arg;
	nrecv = tcparg->nrecv;
	mystream = tcparg->mystream;
	nrecv->tcpReceiver( mystream );
	return 0;
}

// thread argument for the upd recevier
struct UdpArg
{
	NRecv *nrecv;
	Interface *myiface;
};

// udp receiver thread
void *udpReceiverThread( void *arg )
{
	NRecv *nrecv;
	UdpArg *udparg;
	Interface *myiface;
	
	udparg = ( UdpArg * )arg;
	nrecv = udparg->nrecv;
	myiface = udparg->myiface;
	nrecv->udpReceiver( myiface );
	return 0;
}

// statistics collector thread
void *statCollectorThread( void *arg )
{
	NRecv *nrecv;
	
	nrecv = ( NRecv * )arg;
	nrecv->statCollector();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
RStream::RStream()
{
	memset( ( void * )this, 0, sizeof( RStream ) );
	actualStat = &stat1;
	standbyStat = &stat2;
}
	
RStream::~RStream()
{
	if( srcIpRange ) delete srcIpRange;
	if( dstIpRange ) delete dstIpRange;
	if( srcPortRange ) delete srcPortRange;
	if( dstPortRange ) delete dstPortRange;
};

///////////////////////////////////////////////////////////////////////////////
NRecv::NRecv()
{
	pthread_t tid;
	
	run = 0;
	command_mode = 0;
	measure_delay = 0;
	measure_jitter = 0;
	measure_percentile = 0;
	log = 0;
	writestat = 0;
	writelog = 0;
	timestamp = 0;
	p1 = p2 = p3 = 0;
	rt = checkRT();
	results = 0;
	
	if( getuid() )
	{
		error( "nrecv must run with root user id" );
		exit( -1 );
	}
	
	tid = pthread_self();
	interval.tv_sec = 1;
	interval.tv_usec = 0;
	pthread_rwlock_init( &lock, 0 );
	streams.threadSafe();
	ifaces.threadSafe();
	
	// block SIGINT and SIGALRM, needed for sigwait
	sigemptyset( &sigset );
	sigaddset( &sigset, SIGALRM );
 	sigaddset( &sigset, SIGINT );
 	sigaddset( &sigset, SIGTERM );
	pthread_sigmask ( SIG_BLOCK, &sigset, 0 );

	if( pthread_create( &tid, 0, statCollectorThread, ( void * )this ) )
	{
		throw rerr + "cannot create the stat collector thread";
	}
	
	mgmtsock = socket( PF_INET, SOCK_DGRAM, 0 );
}


///////////////////////////////////////////////////////////////////////////////
void NRecv::start()
{
	RStream *mystream;
	
	if( run ) return;
	run = -1;
	initStatfile();
	if( !command_mode )
	{
		if( !streams.size() )
		{
			error( "no active streams" );
			exit( -1 );
		}
	}
	// clear the stats
	streams.begin();
	while( ( mystream = streams.next() ) )
	{
		mystream->stat1.clear();
		mystream->stat2.clear();
		mystream->firstPacket = -1;
	}
	// clear the results
	if( results )
	{
		results->data.wrlock();
		results->data.clear();
		results->data.unlock();
	}
	// start streams
	streams.begin();
	while( ( mystream = streams.next() ) )
	{
		startStream( mystream );
	}
	startTimer();
	gettimeofday( &lastStat, 0 );
	if( !command_mode ) pause();
}


///////////////////////////////////////////////////////////////////////////////
void NRecv::stop()
{
	RStream *mystream;
	Interface *myiface;
	if( !run )
	{
	 return;
	}
	run = 0;
	stopTimer();
	// kill threads and remove streams
	ifaces.begin();
	while( ( myiface = ifaces.next() ) )
	{
		pthread_rwlock_wrlock( &lock );
		pthread_cancel( myiface->tid );
		*( myiface->kill ) = -1;
		pthread_rwlock_unlock( &lock );
		close( myiface->sock );
	}
	ifaces.clear();
	streams.begin();
	while( ( mystream = streams.next() ) )
	{
		if( mystream->proto == IPPROTO_TCP )
		{
	 		pthread_rwlock_wrlock( &lock );
			pthread_cancel( mystream->tid );
			*( mystream->kill ) = -1;
	 		pthread_rwlock_unlock( &lock );
			close( mystream->sock );
			close( mystream->accsock );
		}
		else if( mystream->mcast )
		{
			dropMembership( mystream );
		}
	}
	if( writestat ) statfile.flush();
	if( writelog ) logfile.flush();
}


///////////////////////////////////////////////////////////////////////////////
void NRecv::statCollector()
{
	ostringstream ostream;
	int n;
	unsigned long timediff;
	struct RStream *mystream;
	struct Statistics *mystat;
	struct timeval now;
	struct tm *date;
	unsigned long delay, jitter, minDelay, maxDelay, perc;
	float rate, totalrate;
	unsigned long totalpackets, totallosses, totalmisords;
	int width;
	string *mystring;
	Interface *myiface;
	
	// initialization
	delay = jitter = 0;
	if( timestamp ) width = ID_CLI_SIZE + 18;
	else width = ID_CLI_SIZE;
	gettimeofday( &lastStat, NULL );

	// wait for SIGALRM and SIGINT in infinite while cycle
	while( -1 )
	{
		sigwait( &sigset, &n );
		pthread_rwlock_wrlock( &lock );
		gettimeofday( &now, NULL );   // the starting time of the new statistical interval
		date = localtime( &now.tv_sec );
		if( n == SIGINT || n == SIGTERM )  // quit
		{
			if( writestat ) statfile.close();
			if( writelog ) logfile.close();
			// set back the original promiscuos mode for all interfaces
			ifaces.first();
			while( !ifaces.end() )
			{
				myiface = ifaces.get();
				ifaces.next();
				if( !myiface->promisc )
				{
					setPromiscMode( myiface->ifname, 0 );
				}
			}
			exit( 0 );
		}
		else  // print the stats
		{
			// Replace the actual and the standby containers
			streams.begin();
			while( ( mystream = streams.next() ) )
			{
				if( mystream->terminated ) continue;
				mystat = mystream->actualStat;
				mystream->actualStat = mystream->standbyStat;
				mystream->standbyStat = mystat;
			}
			pthread_rwlock_unlock( &lock );
			totalrate = totalpackets = totallosses = totalmisords = 0;
			ostream.str( "" );
			ostream << fixed;
			if( command_mode and !streams.empty() )
			{
				ostream << "OK time=" << setiosflags( ios::right ) << setfill( '0' ) \
				<< setw( 2 ) << date->tm_year-100 << "/" \
				<< setw( 2 ) <<date->tm_mon+1 << "/" << setw( 2 ) << date->tm_mday << "." \
				<< setw( 2 ) << date->tm_hour << ":" << setw( 2 ) << date->tm_min << ":" \
				<< setw( 2 ) << date->tm_sec << resetiosflags( ios::right ) << setfill( ' ' );
			}
			pthread_rwlock_rdlock( &lock );
			streams.begin();
			while( ( mystream = streams.next() ) )
			{
				mystat = mystream->standbyStat;
				timediff = timeDiff( now, lastStat );  // time from the previous stat interval
				rate = mystat->bytes * ( 8000 / ( float )timediff );  // in kbps
				totalrate += rate;
				totalpackets += mystat->packets;
				totallosses += mystat->losses;
				totalmisords += mystat->misorderings;
				minDelay = mystat->minDelay;
				maxDelay = mystat->maxDelay;
				if( mystream->measureJitter )
				{
					jitter = maxDelay - minDelay;
				}
				if( mystream->measureDelay )
				{
					if( mystat->numofdelays != 0 )
					{
						delay = ( unsigned long )( mystat->delay / mystat->numofdelays );
					}
					else
					{
						delay = 0;
					}
				}

				// now print the statistics

				if( command_mode )
				{
					ostream << " | id=" << mystream->id;
					if( mystream->proto == IPPROTO_UDP )
					{
						ostream << " proto=udp";
					}
					else
					{
						ostream << " proto=tcp";
					}
					ostream << " pkts=" << mystat->packets;
					ostream << " loss=" << mystat->losses;
					ostream << " miso=" << mystat->misorderings;
					ostream << " rate=" << setprecision( 3 ) << rate/1000.0;
					if( mystream->measureDelay )
					{
						ostream << " davg=" << setprecision( 1 ) << ( float )delay/1000.0;
					}
					if( mystream->measureJitter )
					{
						ostream << " dmin=" << setprecision( 1 ) << ( float )minDelay/1000.0;
						ostream << " dmax=" << setprecision( 1 ) << ( float )maxDelay/1000.0;
						ostream << " jitter=" << setprecision( 1 ) << ( float )jitter/1000.0;
					}
					if( mystream->measurePercentile )
					{
						if( p1 != 0 )
						{
							perc = mystat->getPercentile( p1 );
							ostream << " p1=" << setprecision( 1 ) << ( float )perc/1000.0;
						}
						if( p2 != 0 )
						{
							perc = mystat->getPercentile( p2 );
							ostream << " p2=" << setprecision( 1 ) << ( float )perc/1000.0;
						}
						if( p3 != 0 )
						{
							perc = mystat->getPercentile( p3 );
							ostream << " p3=" << setprecision( 1 ) << ( float )perc/1000.0;
						}
					}
					if( mystat->invalid || mystat->foreign )
					{
						ostream << " status=";
						if( mystat->foreign ) ostream << "F";
						if( mystat->invalid ) ostream << "C";
					}
				}
				else
				{
					if( timestamp )
					{
						ostream << setiosflags( ios::right ) << setfill( '0' ) << setw( 2 ) << date->tm_year-100 << "/" \
						<< setw( 2 ) <<date->tm_mon+1 << "/" << setw( 2 ) << date->tm_mday << "." \
						<< setw( 2 ) << date->tm_hour << ":" << setw( 2 ) << date->tm_min << ":" \
						<< setw( 2 ) << date->tm_sec << " " << resetiosflags( ios::right ) << setfill( ' ' ) ;
					}
					ostream << setiosflags( ios::left ) << setw( ID_CLI_SIZE ) << mystream->id;
					ostream  << resetiosflags( ios::left ) << setw( 9 ) << mystat->packets \
					<< setw( 9 ) << mystat->losses << setw( 9 ) << mystat->misorderings \
					<< resetiosflags( ios::left ) << setiosflags( ios::fixed );
					if( rate < 1000.0 ) ostream << setw( 9 ) << setprecision( 3 ) << rate/1000.0 << " M";
					else ostream << setw( 8 ) << setprecision( 2 ) << rate/1000.0 << "  M";
					if( mystream->measureDelay )
					{
						ostream << setw( 9 );
						ostream << setprecision( 1 ) << ( float )delay/1000.0;
					}
					if( mystream->measureJitter )
					{
						ostream << setw( 9 );
						ostream << setprecision( 1 ) << ( float )minDelay/1000.0;
						ostream << setw( 9 );
						ostream << setprecision( 1 ) << ( float )maxDelay/1000.0;
						ostream << setw( 9 );
						ostream << setprecision( 1 ) << ( float )jitter/1000.0;
					}
					if( mystream->measurePercentile )
					{
						if( p1 != 0 )
						{
							ostream << setw( 9 );
							perc = mystat->getPercentile( p1 );
							ostream << setprecision( 1 ) << ( float )perc/1000.0;
						}
						if( p2 != 0 )
						{
							perc = mystat->getPercentile( p2 );
							ostream << setw( 9 );
							ostream << setprecision( 1 ) << ( float )perc/1000.0;
						}
						if( p3 != 0 )
						{
							ostream << setw( 9 );
							perc = mystat->getPercentile( p3 );
							ostream << setprecision( 1 ) << ( float )perc/1000.0;
						}
					}
					if( mystat->invalid || mystat->foreign )
					{
						ostream << " ";
						if( mystat->foreign ) ostream << "F";
						if( mystat->invalid ) ostream << "C";
					}
					ostream << endl;
				}
				
				// now clear the standby stat
				mystat->clear();
			}
			pthread_rwlock_unlock( &lock );
			lastStat = now;
			
			if( command_mode )
			{
				// append record to the results list
				ostream << endl;
				mystring = new string;
				*mystring = ostream.str();
				// try to send it
				results->add( mystring );
			}
			else
			{
				// now print the total stats
				if( timestamp )
				{
					ostream << setiosflags( ios::right ) << setfill( '0' ) << setw( 2 ) << date->tm_year-100 << "/" \
					<< setw( 2 ) <<date->tm_mon+1 << "/" << setw( 2 ) << date->tm_mday << "." \
					<< setw( 2 ) << date->tm_hour << ":" << setw( 2 ) << date->tm_min << ":" \
					<< setw( 2 ) << date->tm_sec << " " << resetiosflags( ios::right ) << setfill( ' ' ) ;
				}
				ostream << setiosflags( ios::left ) << setw( width ) << "TOTAL" \
				<< resetiosflags( ios::left ) << setw( 9 ) << totalpackets \
				<< setw( 9 ) << totallosses << setw( 9 ) << totalmisords \
				<< setprecision( 1 ) << setiosflags( ios::showpoint );
				if( totalrate < 1000.0 ) ostream << setw( 9 ) << setprecision( 3 ) << totalrate/1000.0 << " M";
				else ostream << setw( 8 ) << setprecision( 2 ) << totalrate/1000.0 << "  M";
				ostream << endl << setfill( '-' ) << setw( 85 ) << "-" << setfill( ' ' ) << endl;
				cout << ostream.str();
				if( writestat ) statfile << ostream.str();
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
void NRecv::startTimer()
{
	itimer.it_interval = interval;
	itimer.it_value = interval;
	if( setitimer( ITIMER_REAL, &itimer, 0 ) ) throw rerr + "cannot set the timer";
}


///////////////////////////////////////////////////////////////////////////////
void NRecv::stopTimer()
{
	itimer.it_interval.tv_sec = 0;
	itimer.it_interval.tv_usec = 0;
	itimer.it_value.tv_sec = 0;
	itimer.it_value.tv_usec = 0;
	if( setitimer( ITIMER_REAL, &itimer, 0 ) ) throw rerr + "cannot set the timer";
}


///////////////////////////////////////////////////////////////////////////////
// This is called upon an error. The message is either printed on stdout,
// or in command mode inserted into the results linked list.
void NRecv::error( string err, int errnum )
{
	char buf[256];
	string *mystring;
	
	if( command_mode )
	{
		mystring = new string;
		*mystring = "ERR ";
		*mystring += err;
		if( errnum )
		{
			strerror_r( errnum, buf, 256 );
			*mystring += buf;
		}
		*mystring += "\n";
		results->data.wrlock();
		results->add( mystring );
		results->data.unlock();
	}
	else
	{
		cerr << "ERR: " << err << endl;
		exit( -1 );
	}
}


///////////////////////////////////////////////////////////////////////////////
void NRecv::initStream( RStream *mystream )
{
	RStream *streamp;
	
	streams.begin();
	while( ( streamp = streams.next() ) )
	{
		if( !strcmp( streamp->id, mystream->id ) )
		{
			throw rerr + "stream with id " + mystream->id + " alreay exists";
		}
	}
	if( !mystream->par_id ) throw rerr + "id parameter is missing";
	if( !mystream->par_if ) throw rerr + "interface parameters is missing";
	if( mystream->par_proto == 0 ) throw rerr + "proto parameter is missing";
	if( mystream->proto == IPPROTO_TCP )
	{
		if( mystream->par_dstport == 0 ) throw rerr + "dstport parameter is mandatory for TCP streams";
		if( mystream->dstPortRange ) throw rerr + "port range is not for TCP streams";
		if( mystream->par_srcport ) throw rerr + "srcport parameter is not supported for TCP streams";
		if( mystream->par_srcip ) throw rerr + "srcip parameter is not supported for TCP streams";
		if( mystream->par_dstip ) throw rerr + "dstip parameter is not supported for TCP streams";
		if( mystream->measurePercentile ) throw rerr + "percentile measurement is not supported for TCP streams";
		if( mystream->measureDelay ) throw rerr + "delay measurement is not supported for TCP streams";
		if( mystream->measureJitter ) throw rerr + "jitter measurement is not suported for TCP streams";
		if( mystream->mcast ) throw rerr + "mcast is not supported for TCP streams";
	}
	if( ( mystream->log or mystream->logloss ) and !logfile.is_open() )
	{
		throw rerr + "a logfile must be defined for logging";
	}
}


///////////////////////////////////////////////////////////////////////////////
void NRecv::startStream( RStream *mystream )
{
	int x;
	pthread_t tid;
	Interface *myiface;
	UdpArg *udparg;
	TcpArg *tcparg;
	struct ip_mreq mreq;
	unsigned long firstip, actualip, ifaddr;
	
	mystream->seqNum = 0;
	mystream->firstPacket = -1;
	mystream->actualStat->clear();
	mystream->standbyStat->clear();
	if( mystream->proto == IPPROTO_UDP )
	{
		if( mystream->mcast )
		{
		// join the group
			ifaddr = getIfAddr( mystream->ifname );
			if( mystream->dstIpRange )
			{
				firstip = mystream->dstIpRange->getNextIp();
				actualip = firstip;
				while( -1 )
				{
					mreq.imr_multiaddr.s_addr = actualip;
					mreq.imr_interface.s_addr = ifaddr;
					if( setsockopt ( mgmtsock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof( mreq ) ) )
					{
						throw rerr + "cannot join the multicast group: " + strerror( errno );
					}
					actualip = mystream->dstIpRange->getNextIp();
					if( actualip == firstip ) break;
				}
			}
			else
			{
				mreq.imr_multiaddr.s_addr = mystream->dstIp;
				mreq.imr_interface.s_addr = ifaddr;
				if( setsockopt ( mgmtsock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof( mreq ) ) )
				{
					throw rerr + "cannot join the multicast group: " + strerror( errno );
				}
			}
		}
		x = 0;
		ifaces.begin();
		while( ( myiface = ifaces.next() ) )
		{
			if( !strcmp( myiface->ifname, mystream->ifname ) )
			{
				x = -1;  // there is already an entry
				mystream->iface = myiface;
				myiface->streams++;
				break;
			}
		}
		if( x ) return;  // nothing to do
		
		// add the this interface to the list
		myiface = new Interface;
		strncpy( myiface->ifname, mystream->ifname, IFNAMSIZ );
		myiface->streams = 1;
		ifaces.last();
		ifaces.insertAfter( myiface );
		mystream->iface = myiface;
		
		// create a udp receiver thread for this interface
		udparg = new UdpArg;
		udparg->nrecv = this;
		udparg->myiface = myiface;
		if( pthread_create( &tid, NULL, udpReceiverThread, ( void * )udparg ) )
		{
			throw rerr + "cannot create the UDP receiver thread";
		}
	}
	else
	{
		tcparg = new TcpArg;
		tcparg->nrecv = this;
		tcparg->mystream = mystream;
		if( pthread_create( &tid, NULL, tcpReceiverThread, ( void * )tcparg ) != 0 )
		{
			throw rerr + "cannot create a TCP receiver thread";
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// the tcp receiver function
void *NRecv::tcpReceiver( RStream *mystream )
{
	char c, buf[1600];
	int x, buflen, msglen;
	int port;
	struct timeval intime;
	struct sockaddr_in local;
	int kill;
	
	try
	{
		kill = 0;
		mystream->kill = &kill;
		
		buflen = 1600;  // MTU
		mystream->tid = pthread_self();
		mystream->accsock = socket( PF_INET, SOCK_STREAM, 0 );
		if( mystream->accsock == -1 )
		{
			error( "Cannot open TCP socket: ", errno );
			pthread_exit( 0 );
		}
		x = -1;
		if( setsockopt( mystream->accsock, SOL_SOCKET, SO_REUSEADDR, &x, sizeof( x ) ) )
		{
			error( "Cannot set SO_REUSEADDR socket option: ", errno );
			pthread_exit( 0 );
		}
		x = -1;
		port = mystream->dstPort;
		local.sin_family = PF_INET;
		local.sin_port = htons( port );
		local.sin_addr.s_addr = getIfAddr( mystream->ifname );
		if( bind( mystream->accsock, ( struct sockaddr * )&local, sizeof( local ) ) == -1 )
		{
			error( "Cannot bind socket: ", errno );
			pthread_exit( 0 );
		}
		if( listen( mystream->accsock, 256 ) == -1 )
		{
			error( "Cannot listen on socket: ", errno );
			pthread_exit( 0 );
		}
		while( -1 )
		{
			mystream->sock = accept( mystream->accsock, NULL, NULL );
			if( mystream->sock == -1 )
			{
				error( "Cannot accept: ", errno );
				pthread_exit( 0 );
			}
			c = 0;
			if( mystream->par_prec )
			{
				c = ( char )( mystream->prec << 5 );
			}
			else if( mystream->par_tos )
			{
				c = ( char )mystream->tos;
			}
			if( c )
			{
				if( setsockopt( mystream->sock, SOL_IP, IP_TOS, ( void * )&c, sizeof( c ) ) )
				{
					error( "Cannot set TOS for TCP socket: ", errno );
					pthread_exit( 0 );
				}
			}
			mystream->terminated = 0;
		
			// now start receiving
			
			while( -1 )
			{
				msglen = recv( mystream->sock, buf, buflen, 0 );
				gettimeofday( &intime, NULL );
				if( msglen < 1 )  // reading error
				{
					mystream->terminated = -1;
					close( mystream->sock );
					break;  // break and start accepting again
				}
				pthread_rwlock_rdlock( &lock );
				// check kill state
				if( *( mystream->kill ) )
				{
					pthread_rwlock_unlock( &lock );
					pthread_exit( 0 );
				}
				processPacket( intime, mystream, buf, msglen );
				pthread_rwlock_unlock( &lock );
			}
		}
	}
	catch( string e )
	{
		error( e );
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// the udp receiver function
void *NRecv::udpReceiver( Interface *myiface )
{
	char buf[9100];
	char *ipbuf;
	int buflen, msglen, vlan;
	int i;
	socklen_t addrlen;
	int srcport, dstport, tos, proto, pbits = 0;
	unsigned long srcip, dstip;
	struct timeval intime;
	struct iphdr *iph;
	struct udphdr *udph;
	struct sockaddr_ll addrll;
	struct vlanhdr *vlanh;
	struct sched_param sch_param;
	RStream *mystream;
	struct msghdr msg;
	struct iovec vec;
	struct cmsghdr *cmsg;
	char cmsgbuf[1024];
	int kill;
	
	try
	{
		kill = 0;
		myiface->kill = &kill;
		if( rt )
		{
			sch_param.sched_priority = 99;
			if(  pthread_setschedparam( pthread_self(), SCHED_FIFO, &sch_param ) )
			{
				error( "Cannot set priority scheduling: ", errno );
				pthread_exit( NULL );
			}
		}
		buflen = 1500;  // maximum packet size
		addrlen = sizeof( struct sockaddr_ll );
		myiface->tid = pthread_self();
		myiface->sock = socket( PF_PACKET, SOCK_DGRAM, htons( ETH_P_ALL ) );  // packet socket to capture everything
		if( myiface->sock == -1 )
		{
			error( "Cannot create the packet socket for the UDP streams: ", errno );
			pthread_exit( 0 );
		}
		// now get the flags for the interface and set promiscuous mode
		getPromiscMode( myiface->ifname, myiface->promisc );
		if( !myiface->promisc )  // we have to set promisc mode
		{
			setPromiscMode( myiface->ifname, -1 );
		}
		memset( &addrll, 0, sizeof( addrll ) );
		try
		{
			addrll.sll_ifindex = getIfIndex( myiface->ifname );
		}
		catch( char const *e )
		{
			error( e, errno );
		}
		addrll.sll_family = AF_PACKET;
		addrll.sll_protocol = htons( ETH_P_ALL );
		if( bind( myiface->sock, ( struct sockaddr * )&addrll, sizeof( struct sockaddr_ll ) ) )
		{
			error( "Cannot bind UDP socket to the interface", errno );
			pthread_exit( 0 );
		}
		i = -1;
		if( setsockopt ( myiface->sock, SOL_SOCKET, SO_TIMESTAMP, ( void * )&i, sizeof( i ) ) )
		{
			throw rerr + "cannot set SO_TIMESTAMP socket option";
		}
		memset( &msg, 0, sizeof( msg ) );
		msg.msg_name = &addrll;
		msg.msg_namelen = sizeof( addrll );
		msg.msg_control = cmsgbuf;
		msg.msg_controllen = sizeof( cmsgbuf );
		vec.iov_base = buf;
		vec.iov_len = sizeof( buf );
		msg.msg_iov = &vec;
		msg.msg_iovlen = 1;
	
		// now set the scheduling parameters
	
		sch_param.sched_priority = 99;
		if(  pthread_setschedparam( pthread_self(), SCHED_FIFO, &sch_param ) )
		{
			error( "Cannot set priority scheduling: ", errno );
			pthread_exit( 0 );
		}
		
		// now start capturing
		
		while( -1 )
		{
			msglen = recvmsg( myiface->sock, &msg, 0 );
			if( addrll.sll_pkttype == PACKET_OUTGOING ) continue;  // outgoing frame
			for( cmsg = CMSG_FIRSTHDR( &msg ); cmsg != 0; cmsg = CMSG_NXTHDR( &msg, cmsg ) )
			{
				if( cmsg->cmsg_type == SO_TIMESTAMP )
				{
					intime = *( struct timeval * )CMSG_DATA( cmsg );
					break;
				}
			}
			if( msglen < 1 )  // reading error
			{
				error( "Reading error on the UDP socket: ", errno );
				pthread_exit( 0 );
			}
			if( addrll.sll_protocol == htons( ETH_P_IP ) )
			{
				vlan = -1;
				ipbuf = buf;
			}
			else if( addrll.sll_protocol == htons( 0x8100 ) )
			{
				vlanh = ( struct vlanhdr *)&buf;
				pbits = vlanh->myunion.mystruct.prio;
				vlanh->myunion.mystruct.cfi = 0;
				vlanh->myunion.mystruct.prio = 0;
				vlan = htons( vlanh->myunion.id );
				ipbuf = buf + 4;
			}
			else
			{
				continue;  // some other protocol
			}
			iph = ( struct iphdr * )ipbuf;
			proto = iph->protocol;
			if( proto != IPPROTO_UDP ) continue;
			srcip = iph->saddr;
			dstip = iph->daddr;
			tos = iph->tos;
			udph = ( struct udphdr * )( ipbuf + iph->ihl*4 );
			srcport = ntohs( udph->source );
			dstport = ntohs( udph->dest );
			pthread_rwlock_rdlock( &lock );
			// check kill state
			if( *( myiface->kill ) )
			{
				pthread_rwlock_unlock( &lock );
				pthread_exit( 0 );
			}
			streams.begin();
			while( ( mystream = streams.next() ) )
			{
				if( mystream->proto != IPPROTO_UDP ) continue;
				if( !mystream->iface ) continue;  // has not been initialized yet
				if( mystream->par_dstport )
				{
					if( mystream->dstPortRange == 0 )  // fix port
					{
						if( dstport != mystream->dstPort ) continue;
					}
					else  // port range
					{
						if( dstport < mystream->dstPortRange->getFirstPort() || \
						dstport > mystream->dstPortRange->getLastPort() ) continue; // not in range
						if( ( dstport - mystream->dstPortRange->getFirstPort() ) % \
						mystream->dstPortRange->getIncrement() != 0 ) continue;  // not in range
					}
				}
				if( mystream->par_dstip )
				{
					if( mystream->dstIpRange == 0 )  // fix address
					{
						if( dstip != mystream->dstIp ) continue;
					}
					else  // address range
					{
						if( ntohl( dstip ) < mystream->dstIpRange->getFirstIp() || \
						ntohl( dstip ) > mystream->dstIpRange->getLastIp() ) continue; // not in range
						if( ( ntohl( dstip ) - mystream->dstIpRange->getFirstIp() ) % \
						mystream->dstIpRange->getIncrement() != 0 ) continue;  // not in range
					}
				}
				if( ( vlan != -1 ) && !mystream->par_vlan ) continue;
				if( mystream->iface->tid != myiface->tid ) continue;
				if( mystream->par_tos && ( tos != mystream->tos ) ) continue;
				if( mystream->par_prec && ( tos >> 5 != mystream->prec ) ) continue;
				if( mystream->par_pbits && ( pbits != mystream->pbits ) ) continue;
				if( mystream->par_vlan && ( vlan != mystream->vlan ) ) continue;
				if( mystream->par_srcip )
				{
					if( mystream->srcIpRange == 0 )  // fix address
					{
						if( srcip != mystream->srcIp ) continue;
					}
					else  // address range
					{
						if( ntohl( srcip ) < mystream->srcIpRange->getFirstIp() || \
						ntohl( srcip ) > mystream->srcIpRange->getLastIp() ) continue; // not in range
						if( ( ntohl( srcip ) - mystream->srcIpRange->getFirstIp() ) % \
						mystream->srcIpRange->getIncrement() != 0 ) continue;  // not in range
					}
				}
				if( mystream->par_srcport )
				{
					if( mystream->srcPortRange == 0 )  // fix port
					{
						if( srcport != mystream->srcPort ) continue;
					}
					else  // port range
					{
						if( srcport < mystream->srcPortRange->getFirstPort() || \
						srcport > mystream->srcPortRange->getLastPort() ) continue; // not in range
						if( ( srcport - mystream->srcPortRange->getFirstPort() ) % \
						mystream->srcPortRange->getIncrement() != 0 ) continue;  // not in range
					}
				}
				processPacket( intime, mystream, ( char * )( udph ) + 8, msglen );
			}
			pthread_rwlock_unlock( &lock );
		}
	}
	catch( string e )
	{
		error( e );
		pthread_exit( 0 );
	}
}


///////////////////////////////////////////////////////////////////////////////
// This is called whenever an expected packets is received to update the stats
void NRecv::processPacket( struct timeval &intime, RStream *mystream, char *buf, int msglen )
{
	int bytes, loss;
	int invalid;
	unsigned long *ubuf;
	unsigned long seqnum, delay;
	struct timeval outtime, delaytime;
	struct tm *date;
	struct Statistics *mystat;
		
	loss = 0;
	seqnum = 0;
	invalid = 0;
	delay = 0;
	// lock the mutex and update the statistics
	pthread_rwlock_rdlock( &lock );
	mystat = mystream->actualStat;
	if( mystream->firstPacket )
	{
		mystream->firstTime = intime;  // arrival time of the first packet
	}
	if( mystream->proto == IPPROTO_UDP )
	{
		if( strncmp( buf, "ntools", 6 ) )  // foreign packet
		{
			mystat->foreign = -1;
			pthread_rwlock_unlock( &lock );
			return;
		}
		ubuf = ( unsigned long * )( buf+6 ) ;
		seqnum = ntohl( ubuf[0] );
		bytes = msglen + 18;
		
		if( mystream->measureDelay || mystream->measureJitter || mystream->logdelay )
		{
			outtime.tv_sec = ntohl( ubuf[1] );
			outtime.tv_usec = ntohl( ubuf[2] );
			delaytime = intime;
			delaytime -= outtime;  // delay = in - out
			delay = delaytime.tv_sec*1000000 + delaytime.tv_usec;  // in us
			if( delaytime.tv_usec < 0 || delaytime.tv_sec < 0 || delaytime.tv_sec > 60 )
			{
				invalid = -1;  // the delay is invalid!
				delay = 0;
			}
		}
		if( seqnum == 0 ) mystream->firstPacket = -1;  // ngen has been restarted
		if( !mystream->firstPacket )  // it is not the first packet
		{
			loss = seqnum - mystream->seqNum - 1;
			if( loss < 0 )  // this is a misordered or duplicated packet
			{
				mystat->misorderings++;
				delay = 0;  // skip misordered packets for delay stats
			}
			else if( loss > 0 )
			{
				mystat->losses +=  loss;
			}
			if( seqnum > mystream->seqNum ) mystream->seqNum = seqnum;
		}
		else
		{
			mystream->seqNum = seqnum;
		}
		if( invalid )
		{
			mystat->invalid = -1;
		}
		else if( delay != 0 )
		{
			if( mystream->measureDelay )
			{
				mystat->delay += delay;
				mystat->numofdelays++;
			}
			if( mystream->measurePercentile )
			{
				mystat->addDelay( delay );
			}
			if( mystream->measureJitter )
			{
				if( mystat->packets > 0 )
				{
					if( delay < mystat->minDelay ) mystat->minDelay = delay;
					if( delay > mystat->maxDelay ) mystat->maxDelay = delay;
				}
				else
				{
					mystat->minDelay = mystat->maxDelay = delay;
				}
			}
		}
	}
	else  // TCP
	{
		bytes = msglen + 18;  // add the TCP overhead
	}
	mystat->packets++;
	mystat->bytes += bytes;
	mystream->firstPacket = 0;
	pthread_rwlock_unlock( &lock );
	// now do the logging
	if(
		log ||
		mystream->log ||
		( mystream->logdelay && ( delay > mystream->logdelay )  ) ||
		( mystream->logloss && ( loss != 0 ) )
	)
	{
		date = localtime( &intime.tv_sec );
		logfile << setfill( '0' ) << setw( 2 ) << date->tm_year-100 << "/" \
		<< setw( 2 ) << date->tm_mon+1 << "/" << setw( 2 ) << date->tm_mday << "." \
		<< setw( 2 ) << date->tm_hour << ":" << setw( 2 ) << date->tm_min << ":" \
		<< setw( 2 ) << date->tm_sec << "." << setw( 6 ) << intime.tv_usec << setfill( ' ' ) \
		<< resetiosflags( ios::left ) << setw( 10 ) << mystream->id << setw( 7 );
		if( loss > 0 )
		{
			logfile << "LOSS" << setw( 11 ) << seqnum << setw( 11 ) << loss;
		}
		else
		{
			if( loss < -1 )
			{
				logfile << "MISORD";
			}
			else if( loss == -1 )
			{
				logfile << "DUPLIC";
			}
			else
			{
				logfile << "OK";
			}
			logfile << setw( 11 ) << seqnum << setw( 11 ) << "0";
		}
		logfile << setw( 6 ) << bytes << setw( 10 );
		if( invalid ) logfile << "CLOCK";
		else logfile << delay/1000;
		logfile << endl;
	}
}


///////////////////////////////////////////////////////////////////////////////
void NRecv::deleteStream( RStream *mystream )
{
	if( mystream->proto == IPPROTO_TCP )
	{
 		pthread_rwlock_wrlock( &lock );
 		pthread_cancel( mystream->tid );
		*( mystream->kill ) = -1;
		pthread_rwlock_unlock( &lock );
		close( mystream->accsock );
		close( mystream->sock );
	}
	else
	{
		// drop multicast membership
		if( mystream->mcast ) dropMembership( mystream );
		mystream->iface->streams--;
		if( mystream->iface->streams == 0 )  // we need to remove the interface
		{
	 		pthread_rwlock_wrlock( &lock );
			pthread_cancel( mystream->iface->tid );
			*( mystream->iface->kill ) = 0;
	 		pthread_rwlock_unlock( &lock );
			close( mystream->iface->sock );
			ifaces.first();
			while( !ifaces.end() )  // find the interface and remove it
			{
				if( ifaces.get()->tid == mystream->iface->tid )
				{
					ifaces.remove();
					break;
				}
				ifaces.next();
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
void NRecv::dropMembership( RStream *mystream )
{
	struct ip_mreq mreq;
	unsigned long firstip, actualip, ifaddr;
	
	ifaddr = getIfAddr( mystream->ifname );
	if( mystream->dstIpRange )
	{
		firstip = mystream->dstIpRange->getNextIp();
		actualip = firstip;
		while( -1 )
		{
			mreq.imr_multiaddr.s_addr = actualip;
			mreq.imr_interface.s_addr = ifaddr;
			if( setsockopt ( mgmtsock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof( mreq ) ) )
			{
				throw rerr + "cannot leave the multicast group: " + strerror( errno );
			}
			actualip = mystream->dstIpRange->getNextIp();
			if( actualip == firstip ) break;
		}
	}
	else
	{
		mreq.imr_multiaddr.s_addr = mystream->dstIp;
		mreq.imr_interface.s_addr = ifaddr;
		if( setsockopt ( mgmtsock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof( mreq ) ) )
		{
			throw rerr + "cannot leave the multicast group" + strerror( errno );
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Reads configuration from a stream.
int NRecv::processConfig( istream &file )
{
	int x;
	char buf[256];
	int buflen = 256;
	struct in_addr inaddr;
	int par_int, par_p1, par_p2, par_p3, par_level;
	RStream *mystream;
	ostringstream ostream;
	
	par_int = 0;
	par_p1 = par_p2 = par_p3 = par_level = 0;
	
	while( nextWord( file, buf, buflen ) != -1 )  // while there is a new keyword
	{
		if( !strcmp( "interval", buf ) )
		{
			if( par_int ) throw rerr + "interval parameter is duplicated";
			if( nextWord( file, buf, buflen ) ) throw rerr + "unexpected end of file";
			x = readTime( buf ) / 1000000;
			if( x < 1 or x > 1800 ) throw rerr + "interval is invalid: must be between 1 and 1800";
			interval.tv_sec = x;
			par_int = -1;
			if( run ) startTimer();
		}
		else if( !strcmp( "statfile", buf ) )
		{
			if( nextWord( file, buf, buflen ) ) throw rerr + "unexpected end of file";
			if( statfile.is_open() ) statfile.close();
			statfile.open( buf, ios::out );
			if( !statfile )
			{
				throw rerr + "cannot open " + buf;
			}
			writestat = -1;
		}
		else if( !strcmp( "logfile", buf ) )
		{
			if( nextWord( file, buf, buflen ) ) throw rerr + "unexpected end of file";
			if( logfile.is_open() ) logfile.close();
			logfile.open( buf, ios::out );
			if( !logfile ) throw rerr + "cannot open logfile " + buf;
			writelog = -1;
			initLogfile();
		}
		else if( !strcmp( "timestamp", buf ) )
		{
			timestamp = -1;
		}
		else if( !strcmp( "log", buf ) )
		{
			log = -1;
		}
		else if( !strcmp( "p1", buf ) )
		{
			if( par_p1 ) throw rerr + "p1 parameter is duplicated";
			if( nextWord( file, buf, buflen ) ) throw rerr + "unexpected end of file";
			p1 = ( float )atof( buf );
			if( p1 <= 0 ) throw rerr + "invalid percentile: " + buf;
			par_p1 = -1;
		}
		else if( !strcmp( "p2", buf ) )
		{
			if( par_p2 ) throw rerr + "p2 parameter is duplicated";
			if( nextWord( file, buf, buflen ) ) throw rerr + "unexpected end of file";
			p2 = ( float )atof( buf );
			if( p2 <= 0 ) throw rerr + "invalid percentile: " + buf;
			par_p2 = -1;
		}
		else if( !strcmp( "p3", buf ) )
		{
			if( par_p3 ) throw rerr + "p3 parameter is duplicated";
			if( nextWord( file, buf, buflen ) ) throw rerr + "unexpected end of file";
			p3 = ( float )atof( buf );
			if( p3 <= 0 ) throw rerr + "invalid percentile: " + buf;
			par_p3 = -1;
		}
		else if( !strcmp( "start", buf ) )
		{
			start();
		}
		else if( !strcmp( "stop", buf ) )
		{
			stop();
		}
		else if( !strcmp( "clear", buf ) )
		{
			stop();
			streams.clear();
		}
		else if( !strcmp( "delete", buf ) )
		{
			if( nextWord( file, buf, buflen ) )
			{
				throw rerr + "stream id expected after the delete keyword";
			}
			streams.begin();
			while( ( mystream = streams.next() ) )
			{
				if( !strcmp( buf, mystream->id ) ) break;
			}
			if( !mystream )
			{
				throw rerr + "stream " + buf + " does not exist";
			}
			deleteStream( mystream );
			pthread_rwlock_wrlock( &lock );
			streams.remove();
			pthread_rwlock_unlock( &lock );
			if( !streams.size() ) stopTimer();
		}
		else if( !strcmp( "add", buf ) )
		{
			mystream = new RStream;
			if( nextWord( file, buf, 256 ) ) throw rerr + "unexpected end of file";
			if( strcmp( "{", buf ) ) throw rerr + "keyword { is expected";
			
			// now read the parameters of the stream
			
			while( -1 )
			{
				if( nextWord( file, buf, 256 ) ) throw rerr + "unexpected end of file";
				if( !strcmp( "id", buf ) )
				{
					if( mystream->par_id ) throw rerr + "id parameter is duplicated";
					if( nextWord( file, buf, 256 ) ) throw rerr + "unexpected end of file";
					if( command_mode )
					{
						x = ID_SIZE;
					}
					else
					{
						x = ID_CLI_SIZE;
					}
					if( strlen( buf ) > ( unsigned int )x  )
					{
						ostream.str( "" );
						ostream << "stream id can be maximum " << x << " characters: " << buf;
						throw rerr + ostream.str();
					}
					strcpy( mystream->id, buf );
					mystream->par_id = -1;
				}
				else if( !strcmp( "if", buf ) )
				{
					if( mystream->par_if ) throw rerr + "interface parameter is duplicated";
					if( nextWord( file, buf, 256 ) ) throw rerr + "unexpected end of file";
					strncpy( mystream->ifname, buf, IFNAMSIZ );
					mystream->ifname[IFNAMSIZ-1] = 0;
					mystream->par_if = -1;
				}
				else if( !strcmp( "log", buf ) )
				{
					mystream->log = -1;
				}
				else if( !strcmp( "logloss", buf ) )
				{
					mystream->logloss = -1;
				}
				else if( !strcmp( "logdelay", buf ) )
				{
					if( nextWord( file, buf, 256 ) ) throw rerr + "unexpected end of file";
					x = readTime( buf );
					if( x <= 0 ) throw rerr + "invalid logdelay: " + buf;
					mystream->logdelay = x;
				}
				else if( !strcmp( "delay", buf ) )
				{
					mystream->measureDelay = -1;
					measure_delay = -1;
				}
				else if( !strcmp( "jitter", buf ) )
				{
					mystream->measureJitter = -1;
					measure_jitter = -1;
				}
				else if( !strcmp( "percentile", buf ) )
				{
					mystream->measurePercentile = -1;
					measure_percentile = -1;
				}
				else if( !strcmp( "mcast", buf ) )
				{
					mystream->mcast = -1;
				}
				else if( !strcmp( "srcip", buf ) )
				{
					if( mystream->par_srcip ) throw rerr + "srcip parameter is duplicated";
					if( nextWord( file, buf, 256 ) ) throw rerr + "unexpected end of file";
					if( !strcmp( "range", buf ) )
					{
						mystream->srcIpRange = new IpRange;
						if( mystream->srcIpRange == 0 ) throw rerr + "cannot allocate memory";
						readIpRange( *mystream->srcIpRange, file, buf, 256 );
					}
					else
					{
						if( inet_aton( buf, &inaddr ) == 0 ) throw rerr + "invalid srcip: " + buf;
						mystream->srcIp = inaddr.s_addr;
					}
					mystream->par_srcip = -1;
				}
				else if( !strcmp( "dstip", buf ) )
				{
					if( mystream->par_dstip ) throw rerr + "dstip parameter is duplicated";
					if( nextWord( file, buf, 256 ) ) throw rerr + "unexpected end of file";
					if( !strcmp( "range", buf ) )
					{
						mystream->dstIpRange = new IpRange;
						if( mystream->dstIpRange == 0 ) throw rerr + "cannot allocate memory";
						readIpRange( *mystream->dstIpRange, file, buf, 256 );
					}
					else
					{
						if( inet_aton( buf, &inaddr ) == 0 ) throw rerr + "invalid dstip:" + buf;
						mystream->dstIp = inaddr.s_addr;
					}
					mystream->par_dstip = -1;
				}
				else if( !strcmp( "proto", buf ) )
				{
					if( mystream->par_proto ) throw rerr + "proto parameter is duplicated";
					if( nextWord( file, buf, 256 ) ) throw rerr + "unexpected end of file";
					if( !strcmp( "tcp", buf ) ) mystream->proto = IPPROTO_TCP;
					else if( !strcmp( "udp", buf ) )
					{
						mystream->proto = IPPROTO_UDP;
					}
					else throw rerr + "unknown protocol: " + buf;
					mystream->par_proto = -1;
				}
				else if( !strcmp( "tos", buf ) )
				{
					if( mystream->par_tos || mystream->par_prec ) throw rerr + "tos/prec parameter is duplicated";
					if( nextWord( file, buf, 256 ) ) throw rerr + "unexpected end of file";
					x = atoi( buf );
					if( x < 0 || x > 255 ) throw rerr + "invalid tos: " + buf;
					mystream->tos = x;
					mystream->par_tos = -1;
				}
				else if( !strcmp( "prec", buf ) )
				{
					if( mystream->par_tos || mystream->par_prec ) throw rerr + "tos/prec parameter is duplicated";
					if( nextWord( file, buf, 256 ) ) throw rerr + "unexpected end of file";
					x = atoi( buf );
					if( x < 0 || x > 7 ) throw rerr + "invalid precedence: " + buf;
					mystream->prec = x;
					mystream->par_prec = -1;
				}
				else if( !strcmp( "pbits", buf ) )
				{
					if( mystream->par_pbits ) throw rerr + "pbits parameter is duplicated";
					if( nextWord( file, buf, 256 ) ) throw rerr + "unexpected end of file";
					x = atoi( buf );
					if( x < 0 || x > 7 ) throw rerr + "invalid pbits: " + buf;
					mystream->pbits = x;
					mystream->par_pbits = -1;
				}
				else if( !strcmp( "vlan", buf ) )
				{
					if( mystream->par_vlan ) throw rerr + "vlan parameter is duplicated";
					if( nextWord( file, buf, 256 ) ) throw rerr + "unexpected end of file";
					x = atoi( buf );
					if( x < 0 || x > 4095 ) throw rerr + "invalid vlan: " + buf;
					mystream->vlan = x;
					mystream->par_vlan = -1;
				}
				else if( !strcmp( "srcport", buf ) )
				{
					if( mystream->par_srcport ) throw rerr + "srcport parameter is duplicated";
					if( nextWord( file, buf, 256 ) ) throw rerr + "unexpected end of file";
					if( !strcmp( "range", buf ) )
					{
						mystream->srcPortRange = new PortRange;
						if( mystream->srcPortRange == 0 ) throw rerr + "cannot allocate memory";
						readPortRange( *mystream->srcPortRange, file, buf, 256 );
					}
					else
					{
						x = atoi( buf );
						if( x < 0 || x > 65535 ) throw rerr + "invalid srcport: " + buf;
						mystream->srcPort = x;
					}
					mystream->par_srcport = -1;
				}
				else if( !strcmp( "dstport", buf ) )
				{
					if( mystream->par_dstport ) throw rerr + "dstport parameter is duplicated";
					if( nextWord( file, buf, 256 ) ) throw rerr + "unexpected end of file";
					if( !strcmp( "range", buf ) )
					{
						mystream->dstPortRange = new PortRange;
						if( mystream->dstPortRange == 0 ) throw rerr + "cannot allocate memory";
						readPortRange( *mystream->dstPortRange, file, buf, 256 );
					}
					else
					{
						x = atoi( buf );
						if( x < 0 || x > 65535 ) throw rerr + "invalid dstport: " + buf;
						mystream->dstPort = x;
					}
					mystream->par_dstport = -1;
				}
				else if( !strcmp( "}", buf ) ) break;
				else throw rerr + "invalid keyword: " + buf;
			}
			initStream( mystream );
			pthread_rwlock_wrlock( &lock );
			streams.last();
			streams.insertAfter( mystream );
			pthread_rwlock_unlock( &lock );
			if( run and ( streams.size() == 1 ) ) startTimer();
			if( run ) startStream( mystream );
		}
		else throw rerr + "invalid keyword: " + buf;
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
void NRecv::initStatfile()
{
	ostringstream ostream;
	
	if( !command_mode or statfile.is_open() )
	{
		ostream << setiosflags( ios::left );
		if( timestamp )
		{
			ostream << setw( 18 ) << "Timestamp";
		}
		ostream << setw( ID_CLI_SIZE )<< "Stream" \
		<< resetiosflags( ios::left ) << setw( 9 ) << "Packets" << setw( 9 ) \
		<< "Losses" << setw( 9 ) << "Misorder" << setw( 11
		 
		 ) << "Rate";
		if( measure_delay )
		{
			ostream << setw( 9 ) << "Delay";
		}
		if( measure_jitter )
		{
			ostream << setw( 9 ) << "mDelay" << setw( 9 ) << "MDelay" << setw( 9 ) << "Jitter";
		}
		if( measure_percentile )
		{
			if( p1 != 0 ) ostream << setw( 8 ) << p1 << "p";
			if( p2 != 0 ) ostream << setw( 8 ) << p2 << "p";
			if( p3 != 0 ) ostream << setw( 8 ) << p3 << "p";
		}
		ostream << endl;
		if( !command_mode ) cout << ostream.str();
		if( writestat ) statfile << ostream.str();
	}
}


///////////////////////////////////////////////////////////////////////////////
void NRecv::initLogfile()
{
	logfile << setiosflags( ios::left ) << setw( 24 ) << "Timestamp";
	logfile << resetiosflags( ios::left ) << setw( 10 ) << "Stream";
	logfile << setw( 7 ) << "Status" << setw( 11 ) << "Seq";
	logfile << setw( 11 ) << "Loss";
	logfile << setw( 6 ) << "Bytes" << setw( 10 ) << "Delay" << endl;
}
