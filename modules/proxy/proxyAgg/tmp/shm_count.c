
/*  shm_semcount - Parent and child processes increment a counter
 *                 in shared memory using a semaphore for mutual
 *                 exclusion.
 *
 *  This example uses Unix System V semaphores, not POSIX semaphores.
 *
 *  J. Mohr
 *  2005.03.06
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
const int SHM_SIZE = sizeof( int );
const int SHM_MODE = SHM_R | SHM_W; /* open for read/write access */
const int COUNT_MAX = 1000000;
//const int COUNT_MAX = 10;/* for testing */

const int DELAY_COUNT = 1;

int createsem( int initval );
void p( int semid );
void v( int semid );
void fatalsys( char* );


int main()
{
  int shmid;/* a shared memory descriptor (ID) */
  int * shmptr;/* address of shared memory */
  int mutex;/* a semaphore descriptor */
  pid_t pid;
  int i, j;/* loop control variables */


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

  /* Create a child process to access the same shared memory. */
  if ( ( pid = fork()) < 0 )
    fatalsys("fork failed");

  /* BOTH PARENT AND CHILD EXECUTE THE FOLLOWING CODE */
  for ( i = 0; i < COUNT_MAX; i++ )
    {
      /* This is the straightforward way to do it, but is
          unlikely to result in an error due to a race condition
          (without the semaphores, of course). */
      // p( mutex );
      // (*shmptr)++;
      // v( mutex );

      /* This is the way to test whether a critical section exists at
         all.
         We exaggerate the timing of the race condition. */
      p( mutex );
      int val = *shmptr;
      //for ( j = 0; j < DELAY_COUNT; j++ );/* do nothing */
      *shmptr = ++val;
      v( mutex );
    }

  /* Only the parent does this. */
  if ( pid > 0 )
    {
      /* Wait for the child to complete. */
      if ( wait( NULL ) < 0 )
        fatalsys("wait failed");

      /* Print the result. */
      printf("Shared value is %ld.\n", *shmptr );
    }

  /* Detach the shared memory from the virtual address space. */
  if ( shmdt( (void *) shmptr ) == -1 )
    fatalsys("shmdt failed");

   
  /* Remove the shared memory segment set from the system. 
      This is also only done by the parent.
  */
  if ( pid > 0 )
    if ( shmctl( shmid, IPC_RMID, 0 ) == -1 )
      fatalsys("shmctl failed");
   
  exit( EXIT_SUCCESS );
}


/* Create a single semaphore and initialize it. */
int createsem( int initval )
{
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


/* Dijkstra's 'p()' operation:  decrements a binary semaphore.
   This operation is often called 'wait()', but that name clashes
   with an existing system call for a parent to wait for a child.
*/
void p( int semid )
{
  struct sembuf semoparg = { 0, -1, SEM_UNDO };

  if ( semop( semid, &semoparg, 1 ) == -1 )
    fatalsys("failed to decrement semaphore");
}


/* Dijkstra's 'v()' operation:  increments a binary semaphore. 
   This operation is often called 'signal()', but that name clashes
   with an existing system call for sending a signal to a process.
*/
void v( int semid )
{
  struct sembuf semoparg = { 0, 1, SEM_UNDO };

  if ( semop( semid, &semoparg, 1 ) == -1 )
    fatalsys("failed to increment semaphore");
}


void fatalsys( char* msg )
{
  perror( msg );
  exit( EXIT_FAILURE );
}
