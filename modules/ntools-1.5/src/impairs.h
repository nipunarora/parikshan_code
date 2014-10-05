/*************************************************************************

 impairs.h
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/

#ifndef IMPAIRS_H
#define IMPAIRS_H

#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <sys/time.h>

#include "defs.h"
#include "utils.h"

const int SMKEY = 314159265;

class CLoss
{
 public:
	void Init( double );
	int Drop();
	void Dump( const char *prefix = NULL );
	int exist;
 
 private:
	long n, cnt;
};

class BLoss
{
 public:
	void Init( double, long, long, long );
	int Drop( struct timeval );
	void Dump( const char *prefix = NULL );
	int exist;
 
 private:
	struct timeval period;
	struct timeval _period;
	long n, cnt, length, max, dropped;
	long changed, _n, _length, _max;
	struct timeval start, end;
};

struct JittLoss
{
	long jitter;
	long loss;
};

class CJitter
{
 public:
	void Init( long );
	long Value();
	void Dump( const char *prefix = NULL );
	int exist;
 
 private:
	long n, cnt;
	long arr[DELAY_ARR_SIZE];
};

class Jitter
{
 public:
	void Init( long, long, long, double, long, long );
	struct JittLoss Value( struct timeval );
	void Dump( const char *prefix = NULL );
	int exist;
 
 private:
	long jitter, up, keep, n, down, cnt;
	long changed, _jitter, _up, _keep, _n, _down;
	struct timeval start, end, period, _period;
};

class Impairs
{
 public:
	void Init();
	long total, dropped;
	long delay;
	CLoss closs;
	BLoss bloss;
	CJitter cjitter;
	Jitter jitter;
};

#endif
