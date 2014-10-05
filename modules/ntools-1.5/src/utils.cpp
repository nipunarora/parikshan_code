/*************************************************************************
 utils.cpp
 Copyright (C) 2010, Absilion AB, Norbert Vegh
*************************************************************************/

#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 	
#include <netinet/if_ether.h>
#include <net/if.h>

#include "utils.h"

using namespace std;

// for the ioctls
static int sock = socket( PF_INET, SOCK_DGRAM, 0 );

const unsigned long ULONG_HALF = ULONG_MAX/2;

int checkRT()
{
	size_t pos;
	struct utsname myuname;
	
	uname( &myuname );
	string version;
	version = myuname.version;
	pos = version.find( "PREEMPT RT" );
	if( pos == string::npos ) return 0;
	return -1;
}

// reads the next word from the given file, skips the comment preceeded by '#'

int nextWord( istream &file, char *buf, int buflen )
{
	char c;
	int i;
	c = file.get();
	while( -1 )
	{
		while( isspace( c ) && !file.eof() )  // to skip the space characters
		{
			c = file.get();
		}
		if( file.eof() )
		{
			return -1;   // cannot read this element because of eof
		}
		if( c != '#' ) break;   // comment -> skip this line
		while( !file.eof() )
		{
			c = file.get();
			if( c  == '\n' ) break;
		}
		if( file.eof() ) return -1;
		c = file.get();
	}
	i = 0;
	buflen--;
	while( !isspace( c ) && !file.eof() && ( i < buflen) )
	{
		buf[i++] = c;
		c = file.get();
	}
	buf[i] = 0;
	return 0;
}

// prints a time value

void printTime( unsigned long t )
{
	if( t == 0 )
	{
		cout << "0";
	}
	else if( t < 1000 )
	{
		cout << t << "us";
	}
	else if( t < 1000000 )
	{
		cout << t/1000.0 << "ms";
	}
	else
	{
		cout << t/1000000.0 << "s";
	}
}

// read time parameter from the given buffer, and returns it in usec

long readTime( char *buf )
{
	char c;
	unsigned long l;
	
	l = atol( buf );
	c = buf[ strlen( buf ) - 1 ];
	if( c == 'u' )  // it is in us
	{
		return l;
	}
	else if( c == 'm' )  // it is in ms
	{
		return l * 1000;
	}
	else if( !isdigit( c ) )
	{
		return -1;
	}
	else
	{
		return l * 1000000;  // it is in sec
	}
}


// read rate parameter from the given buffer, and returns it in bit per sec

long readRate( const char *buf )
{
	char c;
	unsigned long l;
	
	l = atol( buf );
	c = buf[ strlen( buf ) - 1 ];
	if( c == 'M' )  // it is in Mbps
	{
		return l * 1000000;
	}
	else if( c == 'k' )  // it is in kbps
	{
		return l * 1000;
	}
	else if( !isdigit( c ) )
	{
		return -1;
	}
	else
	{
		return l;  // it is in bps
	}
}


// gets the ip address for the interface

unsigned long getIfAddr( char *ifname )
{
	char buf[1024];
	int x;
	struct ifreq ifdata;
	struct ifconf ifconfig;
	struct sockaddr_in local;
	unsigned long addr;
	
	addr = 0;
	ifconfig.ifc_len = 1500;
	ifconfig.ifc_buf = buf;
	if( ioctl( sock, SIOCGIFCONF, &ifconfig, sizeof( ifconfig ) ) )
	{
		throw string( "Cannot get interface addresses for " ) + ifname;
	}
	x = 0;
	while( -1 )
	{
		if( x*sizeof( struct ifreq ) >= ( unsigned int )ifconfig.ifc_len ) break;
		ifdata = *( ifconfig.ifc_req + x );
		if( !strcmp( ifname, ifdata.ifr_name ) )  // we found it
		{
			memcpy( &local, &ifdata.ifr_addr, sizeof( ifdata.ifr_addr ) );
			addr = local.sin_addr.s_addr;
			break;
		}
		x++;
	}
	return addr;
}


// gets the index of the interface

int getIfIndex( char *ifname )
{
	struct ifreq ifdata;
	
	strncpy( ifdata.ifr_name, ifname, IFNAMSIZ );
	ifdata.ifr_name[IFNAMSIZ-1] = 0;
	if( ioctl( sock, SIOCGIFINDEX, &ifdata, sizeof( ifdata ) ) )
	{
		throw string( "Cannot get the interface index for " ) + ifname;
	}
	return ifdata.ifr_ifindex;
}


// gets the promisc mode of the interface

int getPromiscMode( const char *ifname, int &mode )
{
	struct ifreq ifdata;
	
	strncpy( ifdata.ifr_name, ifname, IFNAMSIZ );
	ifdata.ifr_name[IFNAMSIZ-1] = 0;
	if( ioctl( sock, SIOCGIFFLAGS, &ifdata, sizeof( ifdata ) ) )
	{
		throw string( "Cannot get the interface flags for " ) + ifname;
	}
	mode = ifdata.ifr_flags & IFF_PROMISC;
	return 0;
}


// sets the promisc mode of the interface

int setPromiscMode( const char *ifname, int mode )
{
	struct ifreq ifdata;
	string str;
	
	strncpy( ifdata.ifr_name, ifname, IFNAMSIZ );
	ifdata.ifr_name[IFNAMSIZ-1] = 0;
	if( ioctl( sock, SIOCGIFFLAGS, &ifdata, sizeof( ifdata ) ) )
	{
		throw string( "Cannot get the interface flags for " ) + ifname;
	}
	if( mode )
	{
		ifdata.ifr_flags |= IFF_PROMISC;
	}
	else
	{
		ifdata.ifr_flags ^= IFF_PROMISC;
	}
	if( ioctl( sock, SIOCSIFFLAGS, &ifdata, sizeof( ifdata ) ) )
	{
		throw string( "Cannot set promiscuous mode for " ) + ifname;
	}
	return 0;
}


// computes ip checksum

void computeChecksum( struct iphdr *iph )
{
	int sum, count;
	unsigned short *hdr;
	
	hdr = ( unsigned short * )iph;
	iph->check = 0;
	sum = 0;
	count = 20;
	while( count > 1 )
	{
		sum += *hdr++;
		count -= 2;
	}
	if( count > 0 )	sum += *( unsigned char * )hdr;
	sum = ( sum >> 16 ) + ( sum & 0xffff );
	sum += ( sum >> 16 );
	iph->check = ( unsigned short )~sum;
}


// returns the difference in us between the two provided time value

long timeDiff( struct timeval &tv1, struct timeval &tv2 )
{
	return ( tv1.tv_sec - tv2.tv_sec ) * 1000000 + tv1.tv_usec - tv2.tv_usec;
}

/*
	Calculates the difference between the two tcp sequence numbers.
	A negative number means retransmission for sequence numbers.
	It also considers the overflow of the sequence number.
	It always considers the smaller distance between the numbers,
	e.g. if seqold is 1000, and seqnew is 4000000000, then
	it considers it as retransmission, rather than a new packet.
*/

long seqDiff( unsigned long seq1, unsigned long seq2 )
{
	if( seq1 == seq2 ) return 0;
	if( seq1 > seq2 )
	{
		if( seq1 - seq2 > ULONG_HALF )
		{
			return -( ULONG_MAX + 1 - seq1 + seq2 );
		}
		else
		{
			return seq1 - seq2;
		}
	}
	else
	{
		if( seq2 - seq1 > ULONG_HALF )
		{
			return ULONG_MAX + 1 - seq2 + seq1;
		}
		else
		{
			return seq1 - seq2;
		}
	}
}


// converts the hexadecimal character c into a decimal integer

int hexConverter( char c )
{
	switch( c )
	{
		case '0': return 0;
		case '1': return 1;
		case '2': return 2;
		case '3': return 3;
		case '4': return 4;
		case '5': return 5;
		case '6': return 6;
		case '7': return 7;
		case '8': return 8;
		case '9': return 9;
		case 'a': case 'A' : return 10;
		case 'b': case 'B' : return 11;
		case 'c': case 'C' : return 12;
		case 'd': case 'D' : return 13;
		case 'e': case 'E' : return 14;
		case 'f': case 'F' : return 15;
	}
	return -1;
}
