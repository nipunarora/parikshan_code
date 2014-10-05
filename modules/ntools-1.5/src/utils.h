/*************************************************************************

 utils.h
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/

#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <fstream>
#include <string.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include "range.h"

using namespace std;

// this file contains some utility functions used by the other programs

struct vlanhdr
{
	union
	{
		struct
		{
			unsigned short blank:4;
			unsigned short cfi:1;
			unsigned short prio:3;
		} mystruct;
		unsigned short id;
	} myunion;
	unsigned short type;
};

inline void operator-=( timeval &tv1, const timeval &tv2 );
inline void operator+=( timeval &tv1, const timeval &tv2 );
inline void operator+=( timeval &tv1, const unsigned long inc );

inline timeval operator-( timeval &tv1, const timeval &tv2 );
inline timeval operator+( timeval &tv1, const timeval &tv2 );
inline int operator<( const timeval &tv1, const timeval &tv2 );
inline int operator>( const timeval &tv1, const timeval &tv2 );
inline int operator<=( const timeval &tv1, const timeval &tv2 );
inline int operator>=( const timeval &tv1, const timeval &tv2 );

int checkRT();
int nextWord( istream &file, char *buf, int buflen );
void printTime( unsigned long t );
long readTime( char *buf );
long readRate( const char *buf );
unsigned long getIfAddr( char *ifname );
int getIfIndex( char *ifname );
int getPromiscMode( const char *ifname, int &mode );
int setPromiscMode( const char *ifname, int mode );
void computeChecksum( struct iphdr *iph );
void buildUdpHeader( struct udphdr *udph, int length, unsigned short sport, unsigned short dport );
long timeDiff( struct timeval &tv1, struct timeval &tv2 );
int compareSeq( unsigned long seq1, unsigned long seq2 );
long seqDiff( unsigned long seq1, unsigned long seq2 );
int hexConverter( char c );


inline void operator-=( timeval &tv1, const timeval &tv2 )
{
	tv1.tv_sec -= tv2.tv_sec;
	tv1.tv_usec -= tv2.tv_usec;
	if( tv1.tv_usec < 0 )
	{
		tv1.tv_sec--;
		tv1.tv_usec += 1000000;
	}
	else if( tv1.tv_usec >= 1000000 )
	{
		tv1.tv_sec++;
		tv1.tv_usec -= 1000000;
	}
}

// adds up the two time value, stores the result in the first

inline void operator+=( timeval &tv1, const timeval &tv2 )
{
	tv1.tv_sec += tv2.tv_sec;
	tv1.tv_usec += tv2.tv_usec;
	if( tv1.tv_usec > 1000000 )
	{
		tv1.tv_usec -= 1000000;
		tv1.tv_sec++;
	}
}
// adds 'inc' usec to the provided time value

inline void operator+=( timeval &tv1, const unsigned long inc )
{
	if( inc < 1000000 )
	{
		tv1.tv_usec += inc;
		if( tv1.tv_usec > 1000000 )
		{
			tv1.tv_usec -= 1000000;
			tv1.tv_sec++;
		}
	}
	else
	{
		tv1.tv_usec += inc;
		tv1.tv_sec += tv1.tv_usec/1000000;
		tv1.tv_usec = tv1.tv_usec%1000000;
	}
}

inline timeval operator-( timeval &tv1, const timeval &tv2 )
{
	timeval ret;
	
	ret.tv_sec = tv1.tv_sec - tv2.tv_sec;
	ret.tv_usec = tv1.tv_usec - tv2.tv_usec;
	if( ret.tv_usec < 0 )
	{
		ret.tv_usec += 1000000;
		ret.tv_sec--;
	}
	return ret;
}

inline timeval operator+( timeval &tv1, const timeval &tv2 )
{
	timeval ret;
	
	ret.tv_sec = tv1.tv_sec + tv2.tv_sec;
	ret.tv_usec = tv1.tv_usec + tv2.tv_usec;
	if( ret.tv_usec > 1000000 )
	{
		ret.tv_usec -= 1000000;
		ret.tv_sec++;
	}
	return ret;
}

inline int operator<( const timeval &tv1, const timeval &tv2 )
{
	if( tv1.tv_sec < tv2.tv_sec ) return -1;
	if( ( tv1.tv_sec == tv2.tv_sec ) && ( tv1.tv_usec < tv2.tv_usec ) ) return -1;
	return 0;
}

inline int operator<=( const timeval &tv1, const timeval &tv2 )
{
	if( tv1.tv_sec < tv2.tv_sec ) return -1;
	if( ( tv1.tv_sec == tv2.tv_sec ) && ( tv1.tv_usec <= tv2.tv_usec ) ) return -1;
	return 0;
}

inline int operator>( const timeval &tv1, const timeval &tv2 )
{
	if( tv1.tv_sec > tv2.tv_sec ) return -1;
	if( ( tv1.tv_sec == tv2.tv_sec ) && ( tv1.tv_usec > tv2.tv_usec ) ) return -1;
	return 0;
}

inline int operator>=( const timeval &tv1, const timeval &tv2 )
{
	if( tv1.tv_sec > tv2.tv_sec ) return -1;
	if( ( tv1.tv_sec == tv2.tv_sec ) && ( tv1.tv_usec >= tv2.tv_usec ) ) return -1;
	return 0;
}

#endif
