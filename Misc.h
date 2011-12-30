
/*
 *
 * Multithreaded HTTP Download Accelarator: Aget
 * (c) 2002, Murat Balaban <murat@enderunix.org>
 *
 * See COPYING for copyright and copying restrictions
 *
 */

#ifndef MISC_H
#define MISC_H

#include "Data.h"

#define	LOGSIZ	1024

int calc_offset(long long, int, int);
int numofthreads(int);
void parse_url(char *, struct request *);
void usage();
void revstr(char *);				/* Reverse String				*/
void Log(char *, ...);				/* Log 						*/
void updateProgressBar(float, float);
void handleHttpRetcode(char *);

time_t  t_start, t_finish;

#endif

