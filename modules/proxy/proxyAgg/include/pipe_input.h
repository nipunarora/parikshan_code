/* 
 * 
 * Filename: pipe_input.h
 * Author: Nipun Arora
 * Created: Sun Dec 28 12:56:04 2014 (-0500)
 * URL: http://www.nipunarora.net 
 * 
 * Description: 
 */

#ifndef PIPE_H
#define PIPE_H

#define BUF_SIZE 1024
#define NUM_CONNS 100


void setBufferSize(int fd, int buffer);
void getPipeSize(int pipe_in, int pipe_out);
void getPipeSize2(int pipe_in, int pipe_out); 
void create_pipes();
int setNonblocking(int fd);

#endif
