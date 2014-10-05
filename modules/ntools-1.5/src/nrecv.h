/*************************************************************************

 nrecv.h
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/
#ifndef NRECV_H
#define NRECV_H

#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>

#include "defs.h"
#include "range.h"
#include "utils.h"
#include "statistics.h"
#include "linkedlist.h"

// global variables and definitions

static string rerr = "ERR: ";

struct Interface
{
	char ifname[IFNAMSIZ];
	pthread_t tid;
	int streams;
	int promisc;
	int sock;
	int *kill;  // points to an integer in the thread that is set to -1 if the thread is to be killed
};

// stream specification
struct RStream
{
	RStream();
	~RStream();
	
	int tid;
	int accsock;
	int sock;
	int par_id;
	char id[ID_SIZE+1];
	int par_if;
	char ifname[IFNAMSIZ];
	Interface *iface;
	int log;
	int logloss;
	unsigned long logdelay;
	int par_srcip;
	unsigned long srcIp;  // network byte order
	IpRange *srcIpRange;
	int par_dstip;
	unsigned long dstIp;  // network byte order
	IpRange *dstIpRange;
	int par_proto;
	int proto;
	int par_vlan;
	int vlan;
	int par_srcport;
	int srcPort;  // host byte order
	PortRange *srcPortRange;
	int par_dstport;
	int dstPort;  // host byte order
	PortRange *dstPortRange;
	int par_tos;
	int tos;
	int par_prec;
	int prec;
	int par_pbits;
	int pbits;
	int mcast;  // whether we need to do a join
	int measureDelay;
	int measureJitter;
	int measurePercentile;
	int firstPacket;  // this is set to 0 when receiving the first packet
	unsigned long seqNum;
	struct Statistics *actualStat;
	struct Statistics *standbyStat;
	struct Statistics stat1;
	struct Statistics stat2;
	struct timeval firstTime;  // the arrival time of the first packet
	int terminated;  // indicates that the stream is broken
	int *kill;
};

// the recevier class
class NRecv
{
  public:
	NRecv();
	void start();
	void stop();
	void error( string err, int errnum = 0 );
	int processConfig( istream &file );
	void deleteStream( RStream * );
	void initStream( RStream *mystream );
	void startStream( RStream *mystream );
	void dropMembership( RStream *mystream );
	void processPacket( struct timeval &tv, RStream *, char *buf, int msglen );
	void statCollector();
	void *udpReceiver( Interface * );
	void *tcpReceiver( RStream * );
	void initStatfile();
	void initLogfile();
	void startTimer();
	void stopTimer();
	void commandMode( Results *res )
	{
		command_mode = -1;
		results = res;
	}
	float p1, p2, p3;
	int measure_delay;
	int measure_jitter;
	int measure_percentile;
	int timestamp;
	struct timeval interval;
	int log;  // we need to log all streams
	int writestat;  // whether we need to save results to a stat file
	int writelog;  // whether we need to save results to a log file
	ofstream statfile;
	ofstream logfile;
	LinkedList <RStream> streams;
	
 protected:
	pthread_t tid;
	sigset_t sigset;
	int rt;  // whether the kernel is rt capable
	int run;  // indicates whether the recevier is running
	int mgmtsock;  // management socket for multicast
	Results *results;  // points to the results list of ntoolsd
	int command_mode;  // indicates that the receiver is managed by ntoolsd
	pthread_rwlock_t lock;  // protects the stream list
	struct itimerval itimer;  // timer for calculating statistics
	struct timeval lastStat;  // the last statistical interval
	LinkedList <Interface> ifaces;
};

#endif
