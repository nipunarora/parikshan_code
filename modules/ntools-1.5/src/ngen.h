/*************************************************************************

 ngen.h
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/
#ifndef NGEN_H
#define NGEN_H

#include <pthread.h>
#include <net/if.h>
#include <netpacket/packet.h>

#include "defs.h"
#include "range.h"
#include "distrib.h"
#include "statistics.h"
#include "linkedlist.h"

static string gerr = "ERR: ";

// stream speficiation
struct GStream
{
	GStream();
	~GStream();
	
	char id[ID_SIZE+1];
	pthread_t tid;  // thread id (for TCP streams)
	int sock;  // tcp socket
	char dstMac[13];  // the destination mac address
	char ifname[IFNAMSIZ];  // the sending interface
	struct sockaddr_ll addrll;
	int ifIndex;
	unsigned long srcIp;  // network byte order
	IpRange *srcIpRange;
	unsigned long dstIp;  // network byte order
	IpRange *dstIpRange;
	int proto;
	int vlan;
	int srcPort;  // host byte order
	PortRange *srcPortRange;
	int dstPort;  // host byte order
	PortRange *dstPortRange;
	int tos;
	int pbits;
	Distrib *dist;
	unsigned long seqNum;
	int par_id, par_if, par_pbits, par_dstmac, par_outif;
	int par_srcip, par_dstip, par_proto, par_srcport, par_dstport, par_vlan, par_tos;
};


// the generator class
class NGen
{
  public:
	NGen();
	void error( string err, int errnum = 0 );
	void start();
	void stop();
	void udpSender();
	void tcpSender( GStream *mystream );
	void initStream( GStream *mystream );
	void startStream( GStream *mystream );
	int processConfig( istream &file );
	void commandMode( Results *res )
	{
		command_mode = -1;
		results = res;
	}
	LinkedList <GStream> streams;  // stream table
	
 protected:
	pthread_t tid;
	int rt;  // whether the kernel is rt capable
	int command_mode;  // indicates that the generator is managed by ntoolsd
	int run;  // indicates the state of the generator
	int udpstreams;
	int udprun;  // state of udp sender
	pthread_mutex_t mutex_udprun;
	pthread_cond_t cond_udprun;
	unsigned long defaddr;  // the default interface's address
	Results *results; // points to results list of ntoolsd
};

#endif
