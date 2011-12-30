#ifndef MAIN_H
#define MAIN_H

#include <pthread.h>
#include "Data.h"

char *fullurl;

int nthreads;
int fsuggested = 0;

struct request *req;		/* Download jobs		*/
pthread_t hthread;		/* Helper thread for signals	*/
struct thread_data *wthread;	/* Worker Threads		*/


#endif
