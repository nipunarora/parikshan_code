/*************************************************************************

 range.cpp
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "range.h"
#include "utils.h"

void IpRange::init( unsigned long firstip, unsigned long increment, long number )
{
	first = ntohl( firstip );  // convert to host byte order
	inc = ntohl( increment );
	num = number;
	last = first + ( num-1 )*inc;
	actual = first;
}

unsigned long IpRange::getNextIp()
{
	unsigned long ret;
	
	ret = actual;
	actual += inc;
	if( actual > last ) actual = first;
	return htonl( ret );  // convert back to network byte order
}


void PortRange::init( int firstport, int increment, int number )
{
	first = firstport;
	inc = increment;
	num = number;
	last = first + ( num-1 )*inc;
	actual = first;
}

int PortRange::getNextPort()
{
	int ret;
	
	ret = actual;
	actual += inc;
	if( actual > last ) actual = first;
	return ret;  // returns host byte order
}

// reads the IP range parameters from the config file, and puts it into 'range'

int readIpRange( IpRange &range, istream &file, char *buf, int buflen )
{
	struct in_addr first, increment;
	long number;
	int par_first, par_inc, par_num;
	
	par_first = par_inc = par_num = 0;
	number = 0;
	if( nextWord( file, buf, buflen ) ) throw string ( "Unexpected end of file" );
	if( strcmp( "{", buf ) ) throw string( "{ is expected" );

	// now read the range parameters

	while( -1 )
	{
		if( nextWord( file, buf, buflen ) ) throw string( "Unexpected end of file" );
		if( !strcmp( "first", buf ) )
		{
			if( nextWord( file, buf, buflen ) ) throw string( "Unexpected end of file" );
			if( inet_aton( buf, &first ) == 0 ) throw string( "Invalid first" );
			par_first = -1;
		}
		else if( !strcmp( "inc", buf ) )
		{
			if( nextWord( file, buf, buflen ) ) throw string( "Unexpected end of file" );
			if( inet_aton( buf, &increment ) == 0 ) throw string( "Invalid increment" );
			par_inc = -1;
		}
		else if( !strcmp( "num", buf ) )
		{
			if( nextWord( file, buf, buflen ) ) throw string( "Unexpected end of file" );
			number = atoi( buf );
			if( number < 1 ) throw string( "Invalid number" );
			par_num = -1;
		}
		else if( !strcmp( "}", buf ) ) break;
		else throw string( "Invalid keyword" );
	}
	if( !par_first ) throw string( "first parameter is missing" );
	if( !par_inc ) throw string( "increment parameter is missing" );
	if( !par_num ) throw string( "number parameter is missing" );
	range.init( first.s_addr, increment.s_addr, number );
	return 0;
}


// reads the port range parameters from the config file, and puts it into 'range'

int readPortRange( PortRange &range, istream &file, char *buf, int buflen )
{
	int first, increment, number;
	int par_first, par_inc, par_num;
	
	first = increment = number = 0;
	par_first = par_inc = par_num = 0;
	if( nextWord( file, buf, buflen ) ) throw string( "Unexpected end of file" );
	if( strcmp( "{", buf ) ) throw string( "{ is expected" );

	// now read the range parameters

	while( -1 )
	{
		if( nextWord( file, buf, buflen ) ) throw string( "Unexpected end of file" );
		if( !strcmp( "first", buf ) )
		{
			if( nextWord( file, buf, buflen ) ) throw string( "Unexpected end of file" );
			first = atoi( buf );
			if( first < 0 || first > 65535 ) throw string( "Invalid first" );
			first = first;
			par_first = -1;
		}
		else if( !strcmp( "inc", buf ) )
		{
			if( nextWord( file, buf, buflen ) ) throw string( "Unexpected end of file" );
			increment = atoi( buf );
			if( increment < 0 ) throw string( "Invalid increment" );
			increment = increment;
			par_inc = -1;
		}
		else if( !strcmp( "num", buf ) )
		{
			if( nextWord( file, buf, buflen ) ) throw string( "Unexpected end of file" );
			number = atoi( buf );
			if( number < 1 || number > 65535 ) throw string( "Invalid number" );
			par_num = -1;
		}
		else if( !strcmp( "}", buf ) ) break;
		else throw string( "Invalid keyword" );
	}
	if( !par_first ) throw string( "first parameter is missing" );
	if( !par_inc ) throw string( "increment parameter is missing" );
	if( !par_num ) throw string( "number parameter is missing" );
	range.init( first, increment, number );
	return 0;
}
