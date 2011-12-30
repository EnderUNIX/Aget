#ifndef SOLARIS
#define _XOPEN_SOURCE 500
#endif


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h>


#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#ifdef SOLARIS
#include <arpa/nameser.h>
#include <resolv.h>
#endif


#include "Head.h"
#include "Data.h"
#include "Defs.h"
#include "Misc.h"

#include <errno.h>
extern int errno;
extern int h_errno;

#ifdef SOLARIS
extern const char *hstrerror(int);
#endif

void http_head_req(struct request *req)
{
	struct sockaddr_in sin;
	struct hostent *he;
	int sd;
	char *sbuf;
	char *rbuf;
	char *tok;
	char *s;
	long long clength;


	sbuf = (char *)calloc(HEADREQSIZ + strlen(req->url), sizeof(char));
	rbuf = (char *)calloc(HEADREQSIZ, sizeof(char));

	if ((he = gethostbyname(req->host)) == NULL) {
		Log("Error: Cannot resolve hostname for %s: %s", 
				req->host,
				hstrerror(h_errno));
		exit(1);
	}
	strncpy(req->ip, inet_ntoa(*(struct in_addr *)he->h_addr), MAXIPSIZ);


	time(&t_start);
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr =inet_addr(req->ip);
	sin.sin_port = htons(req->port);

	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		Log("Socket creation failed for Head Request: %s", strerror(errno));
		exit(1);
	}
	if ((connect(sd, (const struct sockaddr *)&sin, sizeof(sin))) == -1) {
		Log("Connection failed for Head Request: %s", strerror(errno));
		exit(1);
	}
	Log("Head-Request Connection established");

	sprintf(sbuf, HEADREQ, req->url, req->host, PROGVERSION);
	if ((send(sd, sbuf, strlen(sbuf), 0)) == -1) {
		Log("send failed for Head Request: %s", strerror(errno));
		exit(1);
	}

	if ((recv(sd, rbuf, HEADREQSIZ, 0)) == -1) {
		Log("recv failed for Head Request: %s", strerror(errno));
		exit(1);
	}

	handleHttpRetcode(rbuf);

	tok = strtok(rbuf, "\r\n");
        if ((strstr(tok, "HTTP/1.1 200")) != NULL) {
                while ((tok = strtok(NULL, "\r\n")) != NULL) {
			if ((strncasecmp(tok, "Content-Length:", 15)) == 0 ) {
                                s = (tok + strlen("Content-Length: "));
                                clength = atoll(s);
				req->clength = clength;
                        }
                }
        }
	free(sbuf);
	free(rbuf);

}
