
#define _XOPEN_SOURCE 500


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>

#include <netinet/in.h>

#include <sys/types.h>

#include <arpa/inet.h>

#include <sys/socket.h>

#include "Head.h"
#include "Data.h"
#include "Defs.h"
#include "Misc.h"
#include "Download.h"

extern sigset_t signal_set;


unsigned int bwritten = 0;
pthread_mutex_t bwritten_mutex = PTHREAD_MUTEX_INITIALIZER;

void * http_get(void *arg) {
	struct thread_data *td;
	int sd;
	char *rbuf, *s;
	long long dr, dw, i; 
	long long foffset;
	pthread_t tid;
	tid = pthread_self();

	/* Block out all signals	*/
	pthread_sigmask(SIG_BLOCK, &signal_set, NULL);

	/* Set Cancellation Type to Asynchronous	*/
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	td = (struct thread_data *)arg;

	foffset = td->foffset;

	rbuf = (char *)calloc(GETRECVSIZ, sizeof(char));


	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		Log("<THREAD #%ld> socket creation failed: %s", tid, strerror(errno));
		pthread_exit((void *)1);
	}

	if ((connect(sd, (const struct sockaddr *)&td->sin, sizeof(struct sockaddr))) == -1) {
		Log("<THREAD #%ld> connection failed: %s", tid, strerror(errno));
		pthread_exit((void *)1);
	}

	if ((send(sd, td->getstr, strlen(td->getstr), 0)) == -1) {
		Log("<THREAD #%ld> send failed: %s", tid, strerror(errno));
		pthread_exit((void *)1);
	}

        if ((dr = recv(sd, rbuf, GETRECVSIZ, 0)) == -1) {
		Log("<THREAD #%ld> recv failed: %s", tid, strerror(errno));
		pthread_exit((void *)1);
        }

	handleHttpRetcode(rbuf);

        if ((strstr(rbuf, "HTTP/1.1 206")) == NULL) {
		fprintf(stderr, "Something unhandled happened, shutting down...\n");
		exit(1);
	}

	s = rbuf;
	i = 0;
	while(1) {
		if (*s == '\n' && *(s - 1) == '\r' && *(s - 2) == '\n' && *(s - 3) == '\r') {
			s++;
			i++;
			break;
		}
		s++;
		i++;
	}
	td->offset = td->soffset;

	if ((dr - i ) > foffset) 
		dw = pwrite(td->fd, s, (foffset - i), td->soffset);
	else
		dw = pwrite(td->fd, s, (dr - i), td->soffset);
	td->offset = td->soffset + dw;


	pthread_mutex_lock(&bwritten_mutex);
	bwritten += dw;
	pthread_mutex_unlock(&bwritten_mutex);

	while (td->offset < foffset) {
		memset(rbuf, GETRECVSIZ, 0);
		dr = recv(sd, rbuf, GETRECVSIZ, 0);
		if ((td->offset + dr) > foffset)
			dw = pwrite(td->fd, rbuf, foffset - td->offset, td->offset);
		else
			dw = pwrite(td->fd, rbuf, dr, td->offset);
		td->offset += dw;
		pthread_mutex_lock(&bwritten_mutex);
		bwritten += dw;
		pthread_mutex_unlock(&bwritten_mutex);
		updateProgressBar(bwritten, td->clength);	
	}

	if (td->offset == td->foffset)
		td->status = STAT_OK;		/* Tell thet download is OK.	*/

	close(sd);

/*        printf("<THREAD #%ld> Part %d completed, leaving thread...\n", tid, td->tind);*/
	pthread_exit(NULL);
	return NULL;
}
