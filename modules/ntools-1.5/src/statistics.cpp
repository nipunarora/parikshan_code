/*************************************************************************

 statistics.cpp
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/

#include <iostream>
#include <stdlib.h>
#include <sys/socket.h>
#include "statistics.h"

using namespace std;

Results::Results()
{
	sock = 0;
}

void Results::add( string *mystring )
{
	data.wrlock();
	if( !sock or ( send( sock, mystring->c_str(), mystring->size(), 0 ) == -1 ) )
	{
		// we need to queue the result
		if( sock )
		{
			close( sock );
			sock = 0;
		}
		data.last();
		data.insertAfter( mystring );
	}
	data.unlock();
}

Statistics::Statistics()
{
	packets = bytes = losses = misorderings = minDelay = maxDelay = invalid = foreign = 0;
	delay = 0;
	sorted = 0;
}

Statistics::~Statistics()
{
	arr.clear();
}

void Statistics::clear()
{
	arr.clear();
	packets = bytes = losses = misorderings = minDelay = maxDelay = invalid = foreign = 0;
	delay = numofdelays = 0;
	sorted = 0;
}

void Statistics::addDelay( unsigned long delay )
{
	arr.push_back( delay );
	sorted = 0;
}

unsigned long Statistics::getPercentile( float p )
{
	unsigned int i;
	unsigned int num;
	
	num = arr.size();
	
	if( num == 0 ) return 0;
	if( !sorted )
	{
		heapSort();
		sorted = -1;
	}
	i = ( int )( p*num/100 );
	if( i >= num - 1 )
	{
		return arr[num-1];
	}
	else
	{
		return( ( arr[i] + arr[i-1] ) / 2 );
	}
}

void Statistics::heapSort()
{
	int i, temp;
	unsigned int num;
	
	num = arr.size();
	for( i = ( num/2 ) - 1; i >= 0; i-- )
	{
		siftDown( i, num );
	}
	
	for (i = num - 1; i >= 1; i-- )
	{
		temp = arr[0];
		arr[0] = arr[i];
		arr[i] = temp;
		siftDown( 0, i-1 );
	}
}


void Statistics::siftDown( int root, int bottom )
{
	int done, maxChild, temp;
	
	done = 0;
	while( ( root*2 <= bottom ) && ( !done ) )
	{
		if( root*2 == bottom )
		{
			maxChild = root * 2;
		}
		else if( arr[root*2] > arr[root*2 + 1] )
		{
			maxChild = root*2;
		}
		else
		{
			maxChild = root*2 + 1;
		}
		if( arr[root] < arr[maxChild] )
		{
			temp = arr[root];
			arr[root] = arr[maxChild];
			arr[maxChild] = temp;
			root = maxChild;
		}
		else
		{
			done = 1;
		}
	}
}
