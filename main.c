#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>


#include "Defs.h"
#include "Data.h"
#include "Misc.h"
#include "Aget.h"
#include "Signal.h"
#include "Resume.h"
#include "main.h"
#include <errno.h>

int main(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	int c, error = 0, ret;
	struct hist_data h;
	int retlog;

	/* Allocate heap for download request	
	 * struct request stores all the information that might be
	 * of interest
	 */
	req = (struct request *)calloc(1, sizeof(struct request));

	/* Only some signals will be emitted	*/
	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGINT);
	sigaddset(&signal_set, SIGALRM);

	/* Block out all signals	*/
	pthread_sigmask(SIG_BLOCK, &signal_set, NULL);

	/* Create a thread for hadling signals	*/
	if ((ret = pthread_create(&hthread, NULL, signal_waiter, NULL)) != 0) {
		fprintf(stderr, "main: cannot create signal_waiter thread: %s, exiting...\n", strerror(errno));
		exit(-1);
	}

	while (!error && (c = getopt(argc,argv,"p:l:n:hfv")) != -1) {
		switch(c) {
			case 'p':
				req->port = atoi(optarg);
				break;
			case 'f':
				fsuggested = 1;
				break;
			case 'l':
				strncpy(req->lfile, optarg, MAXBUFSIZ);
				break;
			case 'n':
				if ((nthreads = atoi(optarg)) > MAXTHREADS) {
					Log("Error: Maximum # of threads allowed is %d\n", MAXTHREADS);
					nthreads = 0;
				}
				break;
			case 'h':
				printf("%s\n", PROGVERSION);
				usage();
				exit(0);
				break;
			case 'v':
				printf("%s\nby Murat BALABAN <murat@enderunix.org>\n", PROGVERSION);
				exit(0);
				break;
			default:
				error = 1;
				usage();
				exit(1);
				break;
		}
	}

	if (error) {
		usage();
		exit(1);
	}

	if (fsuggested == 1 && nthreads == 0) {
		fprintf(stderr, "\nERROR: -f and -n should be used together!, exiting...\n\n");
		usage();
		exit(1);
	}

	if (argc == 2) 		/* If only url is supplied...	*/
		fullurl = strdup(argv[1]);
	else
	if (optind < argc)
		if (argc > 2)
			fullurl = strdup(argv[optind]);
		else {
			usage();
			exit(1);
		}
	else
	if (optind == argc) {
		usage();
		exit(1);
	}
	
	parse_url(fullurl, req);

	/* If a log file for a previous try has been found, read it and
	 * resume the download job (resume_get), otherwise, start with
	 * a clean job (get) 
	 *
	 * Logfile is of the pattern: aget-$file_name.log
	 */
	if ((retlog = read_log(&h)) != -1)
		resume_get(&h);
	else
		get(req);

	return 0;
}
