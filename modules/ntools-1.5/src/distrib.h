/*************************************************************************

 distrib.h
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/
 
#ifndef DISTRIB_H
#define DISTRIB_H

#include <sys/time.h>

// the base class

// distribution types

class Distrib
{
 public:
	virtual ~Distrib();
	virtual int type() = 0;  // to get the type of the class
	virtual void init();  // to initialize the class
	virtual void nextState();  // the get to the next state
	void setSendTime( const struct timeval &tv );  // sets the next sending time
	inline int getSize()  // to get the next packet size
	{
		return nextSize;
	}
	inline struct timeval getSendTime()  // to get the next sending time
	{
		return nextSend;
	}
	const static int TCP = 0;
	const static int FIX = 1;
	const static int ONOFF = 2;
	const static int POISSON = 3;
	
 protected:
	int nextSize;
	struct timeval nextSend;  // next sending time
	unsigned long nextSend_ns;  // ns part of next sending time
};

// the fix class with constant packet size and inter packet time

class Fix : public Distrib
{
 public:
	Fix();
	int type();
	void init();
	void nextState();
	int size;  // bytes
	int rate;  // bit per sec
	
 private:
 	unsigned long period;  // sending period in ns
};


// the onoff class with two states

class OnOff : public Distrib
{
 public:
 	OnOff();
 	int type();
	void init();
	void nextState();
	int rate1;  // bit per sec
	int rate2;
	unsigned long time1;
	unsigned long time2;
	int size1;  // bytes
	int size2;

 private:
	unsigned long period1;  // sending period in ns for state 1
	unsigned long period2;  // sending period in ns for state 2
	struct timeval start;  // the starting time of the actual state
	int state;  // the state ( 1 or 2 )
	int inf;  // if it is not zero, the second state is infinite long
};


// the poisson class with poissonian inter packet time distribution

class Poisson : public Distrib
{
 public:
	Poisson();
	int type();
	void init();
	void nextState();
	inline unsigned long getDifference()  // to get the inter packet time in us
	{
		return diff;
	}
	unsigned long avgint;  // average inter packet time in us (1/lambda)
	int size;  // bytes

 private:
	unsigned long diff;
};
 
#endif
                                                                                                   
