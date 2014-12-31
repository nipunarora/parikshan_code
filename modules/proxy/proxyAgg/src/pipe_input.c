/* 
 * 
 * Filename: pipe_input.c
 * Author: Nipun Arora
 * Created: Sun Dec 28 12:57:23 2014 (-0500)
 * URL: http://www.nipunarora.net 
 * 
 * Description: 
 */

#define _GNU_SOURCE

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <libgen.h>
#include <netdb.h>
#include <resolv.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
#include "input.h"
#include "pipe_input.h"

/* Make file descriptor non blocking */
int setNonblocking(int fd){
  int flags;
  
  /* If they have O_NONBLOCK, use the Posix way to do it */
#if defined(O_NONBLOCK)
  /* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
  if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
    flags = 0;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
  /* Otherwise, use the old way of doing it */
  flags = 1;
  return ioctl(fd, FIOBIO, &flags);
#endif
}     

/* set buffer size to given value */
void setBufferSize(int fd, int buffer){
  DEBUG_PRINT("pipe size before %d \n", fcntl(fd,F_GETPIPE_SZ));

  if(fcntl(fd, F_SETPIPE_SZ, buffer)<0){
    perror("Error in setting pipe size");
  }
  DEBUG_PRINT("pipe size after %d \n", fcntl(fd,F_GETPIPE_SZ));  
}

/* get pipe Buffer size */
void getPipeSize(int pipe_in, int pipe_out){
  int buf_in_size;
  int buf_out_size;
  
  if(ioctl(pipe_in, FIONREAD, &buf_in_size)<0){
    perror("Error \n");
    exit(0);
  }
  printf(" Pipe Buffer IN Size %d \n", (buf_in_size));
  
  if(ioctl(pipe_out, FIONREAD, &buf_out_size)<0){
    perror("Error \n");
    exit(0);
  }
  
  printf(" Pipe Buffer OUT Size %d \n", (buf_out_size));
}


/*
  void getPipeSize2(int pipe_in, int pipe_out){

  struct stat sb;

  if (fstat(pipe_in, &sb) == -1) {
  perror("stat");
  exit(EXIT_FAILURE);
  }
  printf("File size 1: %lld bytes\n", (long long) sb.st_size);


  if (fstat(pipe_out, &sb) == -1) {
  perror("stat");
  exit(EXIT_FAILURE);
  }
  printf("File size 2: %lld bytes\n", (long long) sb.st_size);
  }
*/

