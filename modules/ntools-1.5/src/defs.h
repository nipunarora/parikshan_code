/*************************************************************************

 defs.h
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/

#ifndef DEFS_H
#define DEFS_H

#define DEF_SPORT 8888  // the default source port
#define DEF_WINDOW 65536  // the default TCP window size
#define DEF_FILE "distfile"  // the default distribution file
#define DEF_WAIT 1000  // the default waiting time, 1s
#define DEF_MAX_NUM_OF_NCLIENTS 10  // the default max number of nclient threads
#define MIN_RATE 10000  // the minimum acceptable bitrate (10 kbps)
#define MAX_RATE 1000000000  // the maximum acceptable bitrate (1 Gbps)
#define MAX_THREADS 100  // the maximum nserver/nclient threads

#define ID_SIZE 128  // length of stream IDs
#define ID_CLI_SIZE 8  // length of stream IDs in CLI mode

#define WAITING_TIMELIMIT 100
// The waiting time limit in us for the nemud.
// Below this limit nemud will send the packet immediatelly.

#define DELAY_ARR_SIZE 100000  // the array size for delay values (cjitter and filejitter)

#define MAX_ARRSIZE 1000
// the max size of the distarr used for generating the weighted random file sizes

#define MAX_FILESIZES 100
// the maximum number of file sizes processed from the distribution file

const char VERSION[] = "v1.5, (C) 2002-2010, Norbert Vegh\n";

#define MGMTPORT 3333
#define RESULTPORT 3334

#endif
