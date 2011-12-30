#ifndef SIGNAL_H
#define SIGNAL_H

#include <signal.h>
#include <pthread.h>

sigset_t signal_set;

void * signal_waiter(void *arg);
void sigint_handler(void);
void sigalrm_handler(void);

#endif
