/*************************************************************************

 nemu.cpp
 Copyright (C) 2010, Norbert Vegh
 Norbert Vegh, ntools@norvegh.com

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

*************************************************************************/

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 	
#include <netinet/if_ether.h>
#include <netpacket/packet.h>
#include <errno.h>

#include "defs.h"
#include "utils.h"
#include "impairs.h"

using namespace std;

// function declarations

void usage();

// the main function

int main( int argc, char **argv )
{
	double closs1 = 0;  // continuous loss
	double bloss1 = 0;  // bursty loss
	long blossl1 = 0;  // bursty loss length
	long blossp1 = 0;  // bursty loss period
	long blossm1 = 0;  // bursty loss max dropped frames
	long delay1 = 0;  // delay
	long cjitter1 = 0;  // continuous jitter
	long jitter1 = 0;  // jitter
	long jitteru1 = 0; // jitter up
	long jitterk1 = 0; // jitter keep
	double jitterl1 = 0; // jitter loss
	long jitterd1 = 0; // jitter down
	long jitterp1 = 0;  // jitter period
	double closs2 = 0;  // continuous loss
	double bloss2 = 0;  // bursty loss
	long blossl2 = 0;  // bursty loss length
	long blossp2 = 0;  // bursty loss period
	long blossm2 = 0;  // bursty loss max dropped frames
	long delay2 = 0;  // delay
	long cjitter2 = 0;  // continuous jitter
	long jitter2 = 0;  // jitter
	long jitteru2 = 0; // jitter up
	long jitterk2 = 0; // jitter keep
	double jitterl2 = 0; // jitter loss
	long jitterd2 = 0; // jitter down
	long jitterp2 = 0;  // jitter period
	int par_reset, par_cnt, par_clear, par_get;
	int par_closs1, par_bloss1, par_blossl1, par_blossp1, par_blossm1, par_cjitter1;
	int par_delay1, par_jitter1, par_jitteru1, par_jitterk1, par_jitterd1, par_jitterl1, par_jitterp1;
	int par_closs2, par_bloss2, par_blossl2, par_blossp2, par_blossm2, par_cjitter2;
	int par_delay2, par_jitter2, par_jitterl2, par_jitteru2, par_jitterk2, par_jitterd2, par_jitterp2;
	int smid;
	Impairs *impairs;
	
	cerr << "nemu " << VERSION;

	// process the command line arguments
	
	if( argc < 1 ) usage();
	par_reset = par_cnt = par_clear = par_get = 0;
	par_closs1 = par_bloss1 = par_blossl1 = par_blossp1 = par_blossm1 = par_delay1 = par_cjitter1 = 0;
	par_jitter1 = par_jitterl1 = par_jitteru1 = par_jitterk1 = par_jitterd1 = par_jitterp1 = 0;
	par_closs2 = par_bloss2 = par_blossl2 = par_blossp2 = par_blossm2 = par_delay2 = par_cjitter2 = 0;
	par_jitter2 = par_jitterl2 = par_jitteru2 = par_jitterk2 = par_jitterd2 = par_jitterp2 = 0;
	blossm1 = blossm2 = 0;
	
	try
	{
		while( argc > 1 )
		{
			if( !strcmp( "-reset", argv[1] ) )
			{
				par_reset = -1;
				argc--; argv++;
			}
			else if( !strcmp( "-cnt", argv[1] ) )
			{
				par_cnt = -1;
				argc--; argv++;
			}
			else if( !strcmp( "-clear", argv[1] ) )
			{
				par_clear = -1;
				argc--; argv++;
			}
			else if( !strcmp( "-get", argv[1] ) )
			{
				par_get = -1;
				argc--; argv++;
			}
			else if( !strcmp( "-1closs", argv[1] ) )
			{
				if( par_closs1 ) throw "1closs parameter is duplicated";
				if( argc == 1 ) usage();
				closs1 = atof( argv[2] );
				if( closs1 < 0 ) throw "Invalid 1closs";
				par_closs1 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-1bloss", argv[1] ) )
			{
				if( par_bloss1 ) throw "1bloss parameter is duplicated";
				if( argc == 1 ) usage();
				bloss1 = atof( argv[2] );
				if( bloss1 < 0 ) throw "Invalid 1bloss";
				par_bloss1 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-1blossl", argv[1] ) )
			{
				if( par_blossl1 ) throw "1bloss parameter is duplicated";
				if( argc == 1 ) usage();
				blossl1 = readTime( argv[2] );
				if( blossl1 < 0 ) throw "Invalid 1bloss";
				par_blossl1 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-1blossp", argv[1] ) )
			{
				if( par_blossp1 ) throw "1blossp parameter is duplicated";
				if( argc == 1 ) usage();
				blossp1 = readTime( argv[2] );
				if( blossp1 < 0 ) throw "Invalid 1blossp";
				par_blossp1 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-1blossm", argv[1] ) )
			{
				if( par_blossm1 ) throw "1blossm parameter is duplicated";
				if( argc == 1 ) usage();
				blossm1 = atoi( argv[2] );
				if( blossm1 < 0 ) throw "Invalid 1blossm";
				par_blossm1 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-1delay", argv[1] ) )
			{
				if( par_delay1 ) throw "1delay parameter is duplicated";
				if( argc == 1 ) usage();
				delay1 = readTime( argv[2] );
				if( delay1 < 0 ) throw "Invalid 1delay";
				par_delay1 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-1cjitter", argv[1] ) )
			{
				if( par_cjitter1 ) throw "1cjitter parameter is duplicated";
				if( argc == 1 ) usage();
				cjitter1 = readTime( argv[2] );
				if( cjitter1 < 0 ) throw "Invalid 1cjitter";
				par_cjitter1 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-1jitter", argv[1] ) )
			{
				if( par_jitter1 ) throw "1jitter parameter is duplicated";
				if( argc == 1 ) usage();
				jitter1 = readTime( argv[2] );
				if( jitter1 < 0 ) throw "Invalid 1jitter";
				par_jitter1 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-1jitteru", argv[1] ) )
			{
				if( par_jitteru1 ) throw "1jitteru parameter is duplicated";
				if( argc == 1 ) usage();
				jitteru1 = readTime( argv[2] );
				if( jitteru1 < 0 ) throw "Invalid 1jitteru";
				par_jitteru1 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-1jitterk", argv[1] ) )
			{
				if( par_jitterk1 ) throw "1jitterk parameter is duplicated";
				if( argc == 1 ) usage();
				jitterk1 = readTime( argv[2] );
				if( jitterk1 < 0 ) throw "Invalid 1jitterk";
				par_jitterk1 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-1jitterl", argv[1] ) )
			{
				if( par_jitterl1 ) throw "1jitterl parameter is duplicated";
				if( argc == 1 ) usage();
				jitterl1 = atof( argv[2] );
				if( jitterl1 < 0 ) throw "Invalid 1jitterl";
				par_jitterl1 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-1jitterd", argv[1] ) )
			{
				if( par_jitterd1 ) throw "1jitterd parameter is duplicated";
				if( argc == 1 ) usage();
				jitterd1 = readTime( argv[2] );
				if( jitterd1 < 0 ) throw "Invalid 1jitterd";
				par_jitterd1 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-1jitterp", argv[1] ) )
			{
				if( par_jitterp1 ) throw "1jitterp parameter is duplicated";
				if( argc == 1 ) usage();
				jitterp1 = readTime( argv[2] );
				if( jitterp1 < 0 ) throw "Invalid 1jitterp";
				par_jitterp1 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-2closs", argv[1] ) )
			{
				if( par_closs2 ) throw "2closs parameter is duplicated";
				if( argc == 1 ) usage();
				closs2 = atof( argv[2] );
				if( closs2 < 0 ) throw "Invalid 2closs";
				par_closs2 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-2bloss", argv[1] ) )
			{
				if( par_bloss2 ) throw "2bloss parameter is duplicated";
				if( argc == 1 ) usage();
				bloss2 = atof( argv[2] );
				if( bloss2 < 0 ) throw "Invalid 2bloss";
				par_bloss2 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-2blossl", argv[1] ) )
			{
				if( par_blossl2 ) throw "2blossl parameter is duplicated";
				if( argc == 1 ) usage();
				blossl2 = readTime( argv[2] );
				if( blossl2 < 0 ) throw "Invalid 2blossl";
				par_blossl2 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-2blossp", argv[1] ) )
			{
				if( par_blossp2 ) throw "2blossp parameter is duplicated";
				if( argc == 1 ) usage();
				blossp2 = readTime( argv[2] );
				if( blossp2 < 0 ) throw "Invalid 2blossp";
				par_blossp2 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-2blossm", argv[1] ) )
			{
				if( par_blossm2 ) throw "2blossm parameter is duplicated";
				if( argc == 1 ) usage();
				blossm2 = atoi( argv[2] );
				if( blossm2 < 0 ) throw "Invalid 2blossm";
				par_blossm2 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-2delay", argv[1] ) )
			{
				if( par_delay2 ) throw "2delay parameter is duplicated";
				if( argc == 1 ) usage();
				delay2 = readTime( argv[2] );
				if( delay2 < 0 ) throw "Invalid 2delay";
				par_delay2 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-2cjitter", argv[1] ) )
			{
				if( par_cjitter2 ) throw "2cjitter parameter is duplicated";
				if( argc == 1 ) usage();
				cjitter2 = readTime( argv[2] );
				if( cjitter2 < 0 ) throw "Invalid 2cjitter";
				par_cjitter2 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-2jitter", argv[1] ) )
			{
				if( par_jitter2 ) throw "2jitter parameter is duplicated";
				if( argc == 1 ) usage();
				jitter2 = readTime( argv[2] );
				if( jitter2 < 0 ) throw "Invalid 2jitter";
				par_jitter2 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-2jitteru", argv[1] ) )
			{
				if( par_jitteru2 ) throw "2jitteru parameter is duplicated";
				if( argc == 1 ) usage();
				jitteru2 = readTime( argv[2] );
				if( jitteru2 < 0 ) throw "Invalid 2jitteru";
				par_jitteru2 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-2jitterk", argv[1] ) )
			{
				if( par_jitterk2 ) throw "2jitterk parameter is duplicated";
				if( argc == 1 ) usage();
				jitterk2 = readTime( argv[2] );
				if( jitterk2 < 0 ) throw "Invalid 2jitterk";
				par_jitterk2 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-2jitterl", argv[1] ) )
			{
				if( par_jitterl2 ) throw "2jitterl parameter is duplicated";
				if( argc == 1 ) usage();
				jitterl2 = atof( argv[2] );
				if( jitterl2 < 0 ) throw "Invalid 2jitterl";
				par_jitterl2 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-2jitterd", argv[1] ) )
			{
				if( par_jitterd2 ) throw "2jitterd parameter is duplicated";
				if( argc == 1 ) usage();
				jitterd2 = readTime( argv[2] );
				if( jitterd2 < 0 ) throw "Invalid 2jitterd";
				par_jitterd2 = -1;
				argc -= 2; argv += 2;
			}
			else if( !strcmp( "-2jitterp", argv[1] ) )
			{
				if( par_jitterp2 ) throw "2jitterp parameter is duplicated";
				if( argc == 1 ) usage();
				jitterp2 = readTime( argv[2] );
				if( jitterp2 < 0 ) throw "Invalid 2jitterp";
				par_jitterp2 = -1;
				argc -= 2; argv += 2;
			}
			else
			{
				cerr << "Invalid option: " << argv[1] << endl;
				return -1;
			}
		}
		
		// check parameters
	
		if( par_bloss1 && par_blossl1 && par_blossp1 ) {}
		else if( !par_bloss1 && !par_blossl1 && !par_blossp1 ) {}
		else if( par_bloss1 && !bloss1 )  // need to clear it
		{
			blossl1 = blossm1 = 0;
		}
		else throw "1bloss, 1blossl and 1blossp paramteres must be specified together";
		
		if( par_jitter1 && par_jitterl1 && par_jitterp1 ) {}
		else if( !par_jitter1 && !par_jitterl1 && !par_jitterp1 ) {}
		else if( par_jitter1 && !jitter1 )  // need to clear it
		{
			jitterl1 = jitteru1 = jitterk1 = jitterp1 = 0;
		}
		else throw "1jitter, 1jitterl and 1jitterp paramteres must be specified together";
		
		if( par_bloss2 && par_blossl2 && par_blossp2 ) {}
		else if( !par_bloss2 && !par_blossl2 && !par_blossp2 ) {}
		else if( par_bloss2 && !bloss2 )  // need to clear it
		{
			blossl2 = blossm2 = 0;
		}
		else throw "2bloss, 2blossl and 2blossp paramteres must be specified together";
		
		if( par_jitter2 && par_jitterl2 && par_jitterp2 ) {}
		else if( !par_jitter2 && !par_jitterl2 && !par_jitterp2 ) {}
		else if( par_jitter2 && !jitter2 )  // need to clear it
		{
			jitterl2 = jitteru2 = jitterk2 = jitterp2 = 0;
		}
		else throw "2jitter, 2jitterl and 2jitterp paramteres must be specified together";
	}
	catch( char const *e )
	{
		cerr << e << endl;
		return -1;
	}

	// create shared memory and initalize it
	
	smid = shmget( SMKEY, 2*sizeof( Impairs ), 0 );
	if( smid == -1 )
	{
		perror( "Cannot get shared memory" );
		return -1;
	}
	impairs = ( Impairs * )shmat( smid, NULL, 0 );
	
	// now set the new values
	
	if( par_cnt )
	{
		cout << "1->2 Forwarded: " << impairs[0].total-impairs[0].dropped;
		cout << ", dropped: " << impairs[0].dropped << endl;
		cout << "2->1 Forwarded: " << impairs[1].total-impairs[1].dropped;
		cout << ", dropped: " << impairs[1].dropped << endl;
	}
	if( par_clear )
	{
		impairs[0].total = impairs[1].total = impairs[0].dropped = impairs[1].dropped = 0;
	}
	if( par_get )
	{
		cout << "1->2 delay: ";
		printTime( impairs[0].delay );
		cout << endl;
		impairs[0].closs.Dump( "1->2" );
		impairs[0].bloss.Dump( "1->2" );
		impairs[0].cjitter.Dump( "1->2" );
		impairs[0].jitter.Dump( "1->2" );
		cout << "2->1 delay: ";
		printTime( impairs[1].delay );
		cout << endl;
		impairs[1].closs.Dump( "2->1" );
		impairs[1].bloss.Dump( "2->1" );
		impairs[1].cjitter.Dump( "2->1" );
		impairs[1].jitter.Dump( "2->1" );
	}
	if( par_reset )
	{
		impairs[0].Init();
		impairs[1].Init();
	}
	if( par_delay1 ) impairs[0].delay = delay1;
	if( par_delay2 ) impairs[1].delay = delay2;
	if( par_closs1 ) impairs[0].closs.Init( closs1 );
	if( par_closs2 ) impairs[1].closs.Init( closs2 );
	if( par_bloss1 ) impairs[0].bloss.Init( bloss1, blossl1, blossp1, blossm1 );
	if( par_bloss2 ) impairs[1].bloss.Init( bloss2, blossl2, blossp2, blossm2 );
	if( par_cjitter1 ) impairs[0].cjitter.Init( cjitter1 );
	if( par_cjitter2 ) impairs[1].cjitter.Init( cjitter2 );
	if( par_jitter1 ) impairs[0].jitter.Init( jitter1, jitteru1, jitterk1, jitterl1, jitterd1, jitterp1 );
	if( par_jitter2 ) impairs[1].jitter.Init( jitter2, jitteru2, jitterk2, jitterl2, jitterd2, jitterp2 );
	
	shmdt( impairs );
}


void usage()
{
	cerr << "Command line arguments:\n";
	cerr << "[-reset]\n";
	cerr << "[-clear]\n";
	cerr << "[-cnt]\n";
	cerr << "[-get]\n";
	cerr << "[-1closs continuous_loss]\n";
	cerr << "[-1bloss bursty_loss]\n";
	cerr << "[-1blossm bursty_loss_max]\n";
	cerr << "[-1blossl bursty_loss_lenght]\n";
	cerr << "[-1blossp bursty_loss_period]\n";
	cerr << "[-1delay delay]\n";
	cerr << "[-1cjitter continuous_jitter]\n";
	cerr << "[-1jitter jitter]\n";
	cerr << "[-1jitterl jitter_loss]\n";
	cerr << "[-1jitteru jitter_up]\n";
	cerr << "[-1jitterk jitter_keep]\n";
	cerr << "[-1jitterd jitter_down]\n";
	cerr << "[-1jitterp jitter_period]\n";
	cerr << "[-2closs continuous_loss]\n";
	cerr << "[-2bloss bursty_loss]\n";
	cerr << "[-2blossm bursty_loss_max]\n";
	cerr << "[-2blossl bursty_loss_lenght]\n";
	cerr << "[-2blossp bursty_loss_period]\n";
	cerr << "[-2delay delay]\n";
	cerr << "[-2cjitter continuous_jitter]\n";
	cerr << "[-2jitter jitter]\n";
	cerr << "[-2jitterl jitter_loss]\n";
	cerr << "[-2jitteru jitter_up]\n";
	cerr << "[-2jitterk jitter_keep]\n";
	cerr << "[-2jitterd jitter_down]\n";
	cerr << "[-2jitterp jitter_period]\n";
	exit( -1 );
}
