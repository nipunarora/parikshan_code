/* 
 * Filename: shm_incr.h
 * Author: Nipun Arora
 * Created: Sat Dec 27 20:00:34 2014 (-0500)
 * URL: http://www.nipunarora.net 
 * 
 * Description: 
 */

#ifndef SHM_H
#define SHM_H

int createsem( int initval );
void p( int semid );
void v( int semid );
void fatalsys( char* );
void increment_shm_counter();
void increment_shm_counter2();
int get_shm_counter();
int get_shm_counter2();
void set_shm_counter(int val);
void set_shm_counter2(int val);

#endif
