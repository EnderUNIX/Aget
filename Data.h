#ifndef DATA_H
#define DATA_H

#include <pthread.h>
#include <netinet/in.h>

#include "Defs.h"

typedef struct request {
	char host[MAXHOSTSIZ];		/* Remote host	*/
	char url[MAXURLSIZ];		/* URL		*/
	char file[MAXBUFSIZ];		/* file name	*/
	char lfile[MAXBUFSIZ];		/* if local file name is specified	*/
	char ip[MAXIPSIZ];		/* Remote IP	*/
	char username[MAXBUFSIZ];	
	char password[MAXBUFSIZ];
	int port;
	long long clength;		/* Content-length	*/
	unsigned char proto;		/* Protocol		*/
} request;

typedef struct thread_data {
	struct sockaddr_in sin;
	char getstr[GETREQSIZ];
	long long soffset;		/* Start offset		*/
	long long foffset;		/* Finish offset	*/
	long long offset;		/* Current Offset	*/
	long long clength;		/* Content Length	*/
	int fd;
	pthread_t tid;		/* Thread ID		*/
	unsigned char status;	/* thread exit status	*/
} thread_data;

#endif
