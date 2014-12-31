/* 
 * Filename: input.h
 * Author: Nipun Arora
 * Created: Sat Dec 27 19:30:30 2014 (-0500)
 * URL: http://www.nipunarora.net 
 * 
 * Description: 
 */

#ifndef INPUT_H
#define INPUT_H

#define DEBUG TRUE

#ifdef DEBUG
#define DEBUG_PRINT(fmt, args...)    fprintf(stderr, fmt, ## args)
#else
#define DEBUG_PRINT(fmt, args...)    /* Don't do anything in release builds */
#endif

#define BUF_SIZE 1024
typedef enum {TRUE = 1, FALSE = 0} bool;
int buffer_size;
int server_sock, client_sock, remote_sock, remote_port, duplicate_destination_sock, duplicate_port;

void sigchld_handler(int signal);
void sigterm_handler(int signal);


#endif /** END of HEADER FILE */
