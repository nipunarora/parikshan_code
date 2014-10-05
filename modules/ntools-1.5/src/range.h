/*************************************************************************

 range.h
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/

#ifndef RANGE_H
#define RANGE_H

#include <iostream>
#include <fstream>

using namespace std;

/*
 This class implements an IP address range.
 The range starts from 'firstip', the next addresses are created
 by adding 'increment' to the address. After 'number'-1 increments
 we go back to 'firstip'.
*/

class IpRange
{
 public:
	void init( unsigned long firstip, unsigned long increment, long number );
	unsigned long inline getFirstIp()  // to get the first address
	{
		return first;  // host byte order
	}
	unsigned long getNextIp();  // to get the next address
	unsigned long inline getLastIp()  // to get the last address
	{
		return last;  // host byte order
	}
	unsigned long inline getIncrement()  // to get the increment value
	{
		return inc;
	}

 private:
	unsigned long first, inc, last, actual;
	long num;
};


// the same as the IpRange, but for port ranges

class PortRange
{
 public:
	void init( int startport, int increment, int number );
	int inline getFirstPort()
	{
		return first;
	}
	int getNextPort();
	int inline getLastPort()
	{
		return last;
	}
	unsigned long inline getIncrement()  // to get the increment value
	{
		return inc;
	}

 private:
	int first, inc, actual, last, num;
};

int readIpRange( IpRange &range, istream &file, char *buf, int buflen );
int readPortRange( PortRange &range, istream &file, char *buf, int buflen );

#endif
