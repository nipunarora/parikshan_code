/*************************************************************************

 distrib.cpp
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "distrib.h"
#include "utils.h"

using namespace std;

// the Distrib abstract class

Distrib::~Distrib()
{
}

int Distrib::type() { return 0; }
void Distrib::init() {}
void Distrib::nextState() {};
void Distrib::setSendTime( const struct timeval &tv )
{
	nextSend = tv;
	nextSend_ns = tv.tv_usec * 1000;
};

// fix distribution

Fix::Fix()
{
	rate = 0;
	size = 1522;
}

int Fix::type()
{
	return FIX;
}

void Fix::init()
{
	nextSize = size;
	period = ( unsigned long )ceil( ( size * ( 8000000 / ( float )rate ) * 1000 ) );  // in ns
	gettimeofday( &nextSend, NULL );
	nextSend.tv_sec++;
	nextSend.tv_usec = rand()%1000000;  // randomize the us value
	nextSend_ns = nextSend.tv_usec*1000;
}

void Fix::nextState()
{
	if( rate != -1 )
	{
		nextSend_ns += period;
		nextSend.tv_sec += nextSend_ns/1000000000;
		nextSend_ns %= 1000000000;
		nextSend.tv_usec = nextSend_ns/1000;
	}
}

OnOff::OnOff()
{
	rate1 = rate2 = 0;
	size1 = size2 = 1522;
	time1 = time2 = 0;
}

int OnOff::type()
{
	return ONOFF;
}

void OnOff::init()
{
	gettimeofday( &nextSend, NULL );
	nextSend.tv_sec++;
	nextSend.tv_usec = rand()%1000000;  // randomize the us value
	nextSend_ns = nextSend.tv_usec*1000;
	nextSize = size1;
	start = nextSend;
	state = 1;
	if( rate1 == 0 ) period1 = 0;
	else period1 = ( unsigned long )( size1 * ( 8000000 / ( float )rate1 ) * 1000 );  // in ns
	if( rate2 == 0 ) period2 = 0;
	else period2 = ( unsigned long )( size2 * ( 8000000 / ( float )rate2 ) * 1000 );  // in ns
	if( time2 == 0 ) inf = -1;
	else inf = 0;
	if( period1 == 0 )  // we have to skip the first state
	{
		start += time1;
		nextSend = start;
		nextSize = size2;
		state = 2;
	}
}

void OnOff::nextState()
{
	// calculate the next sending time
	if( state == 1 )
	{
		if( period1 == 0 )  // skip sending for time1 us
		{
			start += time1;
			nextSend = start;
			nextSend_ns = start.tv_usec*1000;
			state = 2;
		}
		else
		{
			nextSend_ns += period1;
			nextSend.tv_sec += nextSend_ns/1000000000;
			nextSend_ns %= 1000000000;
			nextSend.tv_usec = nextSend_ns/1000;
			if( ( unsigned long )timeDiff( nextSend, start ) > time1 )
			{
				state = 2;  // we have to transit to the second state
				start += time1;
				nextSend = start;
				nextSend_ns = start.tv_usec*1000;
			}
		}
	}
	else
	{
		if( period2 == 0 )  // skip sending for time2 us
		{
			start += time2;
			nextSend = start;
			nextSend_ns = start.tv_usec*1000;
			state = 1;
		}
		else
		{
			nextSend_ns += period2;
			nextSend.tv_sec += nextSend_ns/1000000000;
			nextSend_ns %= 1000000000;
			nextSend.tv_usec = nextSend_ns/1000;
			if( !inf && ( ( unsigned long )timeDiff( nextSend, start ) > time2  ) )
			{
				state = 1;  // we have to transit to the first state
				start += time2;
				nextSend = start;
				nextSend_ns = start.tv_usec*1000;
			}
		}
	}

	// calculate the next packet size

	if( state == 1 ) nextSize = size1;
	else nextSize = size2;
}

// the Poisson class

Poisson::Poisson()
{
	size = 1522;
	avgint = 0;
}

int Poisson::type()
{
	return POISSON;
}

void Poisson::init()
{
	diff = 0;
	gettimeofday( &nextSend, NULL );
	srand( nextSend.tv_usec );
	nextSend.tv_sec++;
	nextSend.tv_usec = rand()%1000000;  // randomize the us value
	nextSize = size;
}

void Poisson::nextState()
{
	diff = ( long )( -log( ( float )rand()/( float )RAND_MAX )*( float )avgint );
	nextSend += diff;
};
