
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>



#include "Head.h"
#include "Aget.h"
#include "Misc.h"
#include "Download.h"
#include "Resume.h"
#include "Data.h"


extern struct thread_data *wthread;
extern struct request *req;
extern int fsuggested, nthreads;
extern int bwritten;
extern pthread_t hthread;
#include <errno.h>
extern int errno;


void get(struct request *req)
{
	int i, ret, fd, diff_sec, nok = 0;
	long long soffset;
	char *fmt;
	in_addr_t req_s_addr;
	uint16_t req_sin_port;

	if (req->proto == PROTO_HTTP) 
		http_head_req(req);
	
	/* According to the content-length, get the
	 * suggested number of threads to open.
	 * if the user insists on his value, let it be that way,
	 * use the user's value.
	 */
	ret = numofthreads(req->clength);

	if (fsuggested == 0) {
		if (ret == 0)
			nthreads = 1;
		else
			nthreads = ret;
	}

	wthread = (struct thread_data *)malloc(nthreads * sizeof(struct thread_data));

	Log("Downloading %s (%lld bytes) from site %s(%s:%d). Number of Threads: %d",
			req->url, req->clength, req->host, req->ip, req->port, nthreads);

	if (strlen(req->lfile) != 0) {
		if ((fd = open(req->lfile, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
			fprintf(stderr, "get: cannot open file %s for writing: %s\n", req->lfile, strerror(errno));
			exit(1);
		}
		
	} else {
		if ((fd = open(req->file, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
			fprintf(stderr, "get: cannot open file %s for writing: %s\n", req->lfile, strerror(errno));
			exit(1);
		}
	}

	if ((lseek(fd, req->clength - 1, SEEK_SET)) == -1) {
		fprintf(stderr, "get: couldn't lseek:  %s\n", strerror(errno));
		exit(1);
	}

	if ((write(fd, "0", 1)) == -1) {
		fprintf(stderr, "get: couldn't allocate space for download file: %s\n", strerror(errno));
		exit(1);
	}

	/* Get the starting time, prepare GET format string, and start the threads */
	fmt = (char *)calloc(GETREQSIZ - 2, sizeof(char));
	req_s_addr = inet_addr(req->ip);
	req_sin_port = htons(req->port);
	time(&t_start);
	for (i = 0; i < nthreads; i++) {
		soffset = calc_offset(req->clength, i, nthreads);
		wthread[i].soffset = soffset;
		if (i == nthreads - 1) {
			wthread[i].foffset = req->clength;
		} else {
			wthread[i].foffset = calc_offset(req->clength, i + 1, nthreads);
		}
		wthread[i].sin.sin_family = AF_INET;
		wthread[i].sin.sin_addr.s_addr = req_s_addr;
		wthread[i].sin.sin_port = req_sin_port;
		wthread[i].fd = dup(fd);
		wthread[i].clength = req->clength;
		snprintf(fmt, GETREQSIZ, GETREQ, req->url, req->host, PROGVERSION, soffset);
		strncpy(wthread[i].getstr, fmt, GETREQSIZ);
		pthread_create(&(wthread[i].tid), NULL, http_get, &(wthread[i]));
	}
	free(fmt);


	/* Wait for all of the threads to finish... 
	 * 
	 * TODO: If a thread fails, restart that!
	 */
	for (i = 0; i < nthreads; i++) {
		pthread_join(wthread[i].tid, NULL);
		if (wthread[i].status == STAT_OK)
			nok++;
	}

	if (nok == nthreads) 
		pthread_cancel(hthread);
	else
		pthread_join(hthread, NULL);

	/* Get the finish time, derive some stats	*/
	time(&t_finish);
       	if ((diff_sec = t_finish - t_start) == 0)
		diff_sec = 1;   /* Avoid division by zero       */

	Log("Download completed, job completed in %d seconds. (%lld Kb/sec)",
			diff_sec, (req->clength / diff_sec) / 1024);
        Log("Shutting down...");
	close(fd);
}


void resume_get(struct hist_data *h)
{
	int i, fd, diff_sec, nok = 0;
	char *fmt;

	nthreads = h->nthreads;

	fmt = (char *)calloc(GETREQSIZ - 2, sizeof(char));

	wthread = (struct thread_data *)malloc(nthreads * sizeof(struct thread_data));
	memcpy(req, &h->req, sizeof(struct request));
	memcpy(wthread, h->wthread, sizeof(struct thread_data) * nthreads);

	Log("Resuming download %s (%lld bytes) from site %s(%s:%d). Number of Threads: %d",
			req->url, req->clength, req->host, req->ip, req->port, nthreads);

	if (strlen(req->lfile) != 0) {
		if ((fd = open(req->lfile, O_RDWR, S_IRWXU)) == -1) {
			fprintf(stderr, "get: cannot open file %s for writing: %s\n", req->lfile, strerror(errno));
			exit(1);
		}
		
	} else {
		if ((fd = open(req->file, O_RDWR, S_IRWXU)) == -1) {
			fprintf(stderr, "get: cannot open file %s for writing: %s\n", req->lfile, strerror(errno));
			exit(1);
		}
	}

	time(&t_start);


#ifdef DEBUG
	for (i = 0; i < nthreads; i++)
		printf("Start: %lld, Finish: %lld, Offset: %lld, Diff: %lld\n",
				wthread[i].soffset,
				wthread[i].foffset,
				wthread[i].offset,
				wthread[i].offset - wthread[i].soffset);
#endif

	for (i = 0; i < nthreads; i++) {
		wthread[i].soffset = wthread[i].offset;
		wthread[i].fd = dup(fd);
		snprintf(fmt, GETREQSIZ, GETREQ, req->url, req->host, PROGVERSION, wthread[i].offset);
		strncpy(wthread[i].getstr, fmt, GETREQSIZ);
		pthread_create(&(wthread[i].tid), NULL, http_get, &(wthread[i]));
	}

	for (i = 0; i < nthreads; i++)
		pthread_join(wthread[i].tid, NULL);

	for (i = 0; i < nthreads; i++) {
		pthread_join(wthread[i].tid, NULL);
		if (wthread[i].status == STAT_OK)
			nok++;
	}

	if (nok == nthreads) 
		pthread_cancel(hthread);
	else
		pthread_join(hthread, NULL);



       time(&t_finish);
       if ((diff_sec = t_finish - t_start) == 0)
		diff_sec = 1;   /* Avoid division by zero       */

	Log("Download completed, job completed in %d seconds. (%lld Kb/sec)",
			diff_sec, ((req->clength - h->bwritten) / diff_sec) / 1024);
        Log("Shutting down...");
	close(fd);
}
