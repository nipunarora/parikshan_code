/* 
 * 
 * Filename: shm_incr.c
 * Author: Nipun Arora
 * Created: Sat Dec 27 20:00:52 2014 (-0500)
 * URL: http://www.nipunarora.net 
 * 
 * Description: 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/sem.h>

const int IPC_KEY = IPC_PRIVATE; /* for parent-child sharing only */
const int SHM_SIZE = sizeof( int )*2;
const int SHM_MODE = SHM_R | SHM_W; /* open for read/write access */

int createsem( int initval );
void p( int semid );
void v( int semid );
void fatalsys( char* );
void increment_shm_counter();
int get_shm_counter();

int shmid;/* a shared memory descriptor (ID) */
int * shmptr;/* address of shared memory */
int mutex;/* a semaphore descriptor */


//--> Shared Memory Increment Operations
void create_shm_seg(){
  /* Create a shared memory segment. */
  shmid = shmget( IPC_KEY, SHM_SIZE, SHM_MODE );
  if ( shmid == -1 )
    fatalsys("shmget failed");

  /* Attach the shared memory to the virtual address space. 
      The 0 arguments let the OS decide where to map it.
  */
  shmptr = (int *) shmat( shmid, 0, 0 );

  if ( shmptr == (int *) -1 )
    fatalsys("shmat failed");

  /* Create a semaphore and initialize it to 1. */
  mutex = createsem( 1 );

}

int createsem( int initval ){
   union semun
   {
     int val;// value for SETVAL
     struct semid_ds *buf;// buffer for IPC_STAT & IPC_SET
     unsigned short *array;// array for GETALL & SETALL
     struct seminfo *__buf;// buffer for IPC_INFO
   } semarg;

   /* Create a semaphore set with a single semaphore in it,
      and with read and alter permissions set.
   */
   int semid = semget( IPC_KEY, 1, 0660 );
   if ( semid == -1 )
     fatalsys("semget failed");

   /* Unfortunately, initializing the semaphore is a separate
      operation.
      This initializes our sole semaphore (semaphore 0) to 1.
   */
   semarg.val = initval;
   if ( semctl( semid, 0, SETVAL, semarg ) == -1 )
     fatalsys("semctl SETVAL failed");

   return semid;
}

void p( int semid ){
  struct sembuf semoparg = { 0, -1, SEM_UNDO };

  if ( semop( semid, &semoparg, 1 ) == -1 )
    fatalsys("failed to decrement semaphore");
}

void v( int semid ){
  struct sembuf semoparg = { 0, 1, SEM_UNDO };

  if ( semop( semid, &semoparg, 1 ) == -1 )
    fatalsys("failed to increment semaphore");
}

void fatalsys( char* msg ){
  perror( msg );
  exit( EXIT_FAILURE );
}

/*  increment the shared memory counter */
void increment_shm_counter(){
  p( mutex );
  int val = *shmptr;
  //for ( j = 0; j < DELAY_COUNT; j++ );/* do nothing */
  *shmptr = ++val;
  v( mutex );

}

void increment_shm_counter2(){
  p(mutex);
  int val = *(shmptr+1);
  *(shmptr+1) = ++val;
  v(mutex);
}

/*  get the shared memory counter */
int get_shm_counter(){
  int val = *shmptr;
  return val;
}

int get_shm_counter2(){
  int val = *(shmptr+1);
  return val;
}

void set_shm_counter(int val){
  *shmptr = val;
}

void set_shm_counter2(int val){
  *(shmptr+1) = val;
}
