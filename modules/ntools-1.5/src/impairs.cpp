/*************************************************************************

 impairs.cpp
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/

#include "impairs.h"
#include "utils.h"

using namespace std;

void CLoss::Init( double x )
{
	exist = 0;
	if( x == 0 ) return;
	n = ( long ) ( 100/x );
	cnt = 0;
	if( n > 0 ) exist = -1;
}

int CLoss::Drop()
{
	if( n == 0 ) return 0;
	cnt++;
	if( cnt == n )
	{
		cnt = 0;
		return -1;
	}
	return 0;
}

void:: CLoss::Dump( const char *prefix )
{
	if( prefix != NULL )
	{
		cout << prefix << " closs: ";
	}
	if( exist ) cout << 100.0/( float )n << "%";
	else cout << "0";
	cout << endl;
	
}

void BLoss::Init( double x, long l, long p, long m )
{
	exist = 0;
	if( x == 0 )
	{
		_n = 0;
		_length = 0;
		_period.tv_sec = 0;
		_period.tv_usec = 0;
		_max = 0;
		return;
	}
	_n = ( long ) ( 100/x );
	_length = l;
	_period.tv_sec = p/1000000;
	_period.tv_usec = p%1000000;
	_max = m;
	changed = -1;
	if( _n > 0 ) exist = -1;
}

int BLoss::Drop( struct timeval now )
{
	struct timeval tv;
	
	if( changed )
	{
		n = _n;
		length = _length;
		period = _period;
		max = _max;
		gettimeofday( &start, NULL );
		end = start;
		end += length;
		cnt = 0;
		dropped = 0;
		changed = 0;
	}
	if( n == 0 ) return 0;
	if( now > end )  // we need to adjust the states
	{
		tv = now - end;
		if( tv > period )
		{
			start = now;
			end = now;
			end += length;
		}
		else
		{
			start += period;
			end += period;
		}
		cnt = dropped = 0;
	}
	if( now < start )
	{
		cnt = dropped = 0;
		return 0;  // not in the burst interval
	}
	cnt++;
	if( cnt == n )
	{
		cnt = 0;
		if( ( max != 0 ) && ( dropped == max ) ) return 0;
		dropped++;
		return -1;
	}
	return 0;
}

void:: BLoss::Dump( const char *prefix )
{
	if( prefix != NULL )
	{
		cout << prefix << " ";
	}
	if( exist )
	{
		cout << " bloss: " << 100.0/( float )_n << "%, blossl: ";
		printTime( _length );
		cout << " blossp: ";
		printTime( _period.tv_sec*1000000 + _period.tv_usec );
		cout << ", blossm: " << _max << endl;
	}
	else cout << " bloss: 0" << endl;
}

void CJitter::Init( long x )
{
	long i;
	
	exist = 0;
	n = x;
	cnt = 0;
	if( n ) exist = -1;
	for( i = 0; i < DELAY_ARR_SIZE; i++ )
	{
		arr[i] = ( long )( ( float )n*rand()/( ( float )RAND_MAX ) );
	}
}

long CJitter::Value()
{
	if( n == 0 ) return 0;
	if( cnt == DELAY_ARR_SIZE ) cnt = 0;
	return arr[cnt++];
}

void:: CJitter::Dump( const char *prefix )
{
	if( prefix != NULL )
	{
		cout << prefix << " ";
	}
	cout << "cjitter: ";
	printTime( n );
	cout << endl;
}

void Jitter::Init( long x, long u, long k, double l, long d, long p )
{
	exist = 0;
	_jitter = x;
	_up = u;
	_keep = k;
	if( l == 0 )
	{
		_n = 0;
	}
	else
	{
		_n = ( long ) ( 100/l );
	}
	_down = d;
	_period.tv_sec = p/1000000;
	_period.tv_usec = p%1000000;
	changed = -1;
	if( _jitter ) exist = -1;
}

struct JittLoss Jitter::Value( struct timeval now )
{
	long x;
	struct JittLoss ret;
	struct timeval tv;
	
	if( changed )
	{
		jitter = _jitter;
		up = _up;
		keep = _keep;
		n = _n;
		down = _down;
		period = _period;
		gettimeofday( &start, NULL );
		end = start;
		end += up + keep + down;
		cnt = 0;
		changed = 0;
	}
	if( jitter == 0 )
	{
		ret.loss = 0;
		ret.jitter = 0;
		return ret;
	}
	if( now > end )  // we need to adjust the states
	{
		tv = now - end;
		if( tv > period )
		{
			start = now;
			end = start;
			end += up + keep + down;
		}
		else
		{
			start = start + period;
			end = end + period;
		}
		cnt = 0;
	}
	if( now < start )  // not in jitter interval
	{
		ret.loss = 0;
		ret.jitter = 0;
		return ret;
	}
	x = timeDiff( now, start );
	if( x > up + keep )  // we are in the down phase
	{
		ret.loss = 0;
		
		ret.jitter = ( long )( ( float )jitter/down*( up + keep + down - x ) );
		return ret;
	}
	if( x > up )  // in the keep phase
	{
		ret.loss = 0;
		cnt++;
		if( cnt == n )
		{
			cnt = 0;
			ret.loss = -1;
		}
		ret.jitter = jitter;
		return ret;
	}
	
	// otherwise we are in the up phase
	
	ret.loss = 0;
	ret.jitter = ( long )( ( float )jitter/up*x );
	return ret;
}

void Jitter::Dump( const char *prefix )
{
	if( prefix != NULL )
	{
		cout << prefix << " ";
	}
	if( exist )
	{
		cout << "jitter: ";
		printTime( _jitter );
		cout << ", jitteru: ";
		printTime( _up );
		cout << ", jitterk: ";
		printTime( _keep );
		cout << ", jitterl: " << 100.0/( float )_n << "%, jitterd: ";
		printTime( _down );
		cout << ", jitterp: ";
		printTime( _period.tv_sec*1000000 + _period.tv_usec );
		cout << endl;
	}
	else
	{
		cout << "jitter: 0\n";
	}
}

void Impairs::Init()
{
	total = 0;
	dropped = 0;
	delay = 0;
	closs.Init( 0 );
	bloss.Init( 0, 0, 0, 0 );
	cjitter.Init( 0 );
	jitter.Init( 0, 0, 0, 0, 0, 0 );
}

