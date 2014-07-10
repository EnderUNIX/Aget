#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "Misc.h"
#include "Data.h"

void parse_url(char *url, struct request *req)
{
	char *s;
	int i, j, k;

	i = j = k = 0;
	if ((strncmp(url, "ftp://", 6)) == 0) {
		fprintf(stderr, "Error: Currently Aget doesn't support FTP requests...\n");
		exit(1);
	} else
	if ((strncmp(url, "http://", 7)) != 0) {
		fprintf(stderr, "Error: URL should be of the form http://...\n");
		exit(1);
	}

	if (req->port == 0) {
		req->port = 80;
		req->proto = PROTO_HTTP;
	}


	s = url + 7; 	/* Jump pass http:// part	*/
	for (i = 0; *s != '/'; i++, s++) {
		if (i > MAXHOSTSIZ) {
			fprintf(stderr, "Error: Cannot get hostname from URL...\n");
			exit(1);
		}
		if (*s == ':') {	/* If user/pass is supplied like; http://murat:12345@x.y.com/url.html */
			while(*s != '/') {
				req->username[j++] = *--s;
				i--;
			}
			req->username[--j] = '\0';
			revstr(req->username);
			while(1) {
				if (*s == ':') {
					while(*s != '@') {
						if (k > MAXBUFSIZ) {
							fprintf(stderr, "Error: Cannot get password from URL...\n");
							exit(1);
						}
						req->password[k++] = *++s;
					}
					break;
				}
				s++;
			}
			req->password[--k] = '\0';
		}
		req->host[i] = *s;
	}
	req->host[i] = '\0';
	for (i = 0; *s != '\0'; i++, s++) {
		if (i > MAXURLSIZ) {
			fprintf(stderr, "Error: Cannot get remote file name from URL...\n");
			exit(1);
		}
		req->url[i] = *s;
	}
	req->url[i] = '\0';
	--s;
	for (i = 0; *s != '/'; i++, s--) {
		if (i > MAXBUFSIZ) {
			fprintf(stderr, "Error: Cannot get local file name from URL...\n");
			exit(1);
		}
		req->file[i] = *s;
	}
	req->file[i] = '\0';
	revstr(req->file);

}

int numofthreads(int size)
{
	if (size == 0)
		return 0;
	else if (size < BUFSIZ * 2) 				/* < 16384 */	
		return 1;
	else if ((size >= BUFSIZ * 2) && (size < BUFSIZ * 4))	/* 16384 < x < 32678	*/ 
		return 2;
	else if ((size >= BUFSIZ * 4) && (size < BUFSIZ * 8))	/* 32768 < x < 65536	*/
		return 3;
	else if ((size >= BUFSIZ * 8) && (size < BUFSIZ * 16))	/* 65536 < x < 131072	*/
		return 4;
	else if ((size >= BUFSIZ * 16) && (size < BUFSIZ * 32))	/* 131072 < x < 262144	*/
		return 5;
	else if ((size >= BUFSIZ * 32) && (size < BUFSIZ * 64))	
		return 6;
	else if ((size >= BUFSIZ * 64) && (size < BUFSIZ * 128))	
		return 7;
	else if ((size >= BUFSIZ * 128) && (size < BUFSIZ * 256))	
		return 8;
	else if ((size >= BUFSIZ * 256) && (size < BUFSIZ * 512))	
		return 9;
	else 
		return 10;
}

int calc_offset(long long total, int part, int nthreads)
{
	return (part * (total / nthreads));
}


void usage()
{
	fprintf(stderr, "usage: aget [options] url\n");
	fprintf(stderr, "\toptions:\n");
	fprintf(stderr, "\t\t-p port number\n");
	fprintf(stderr, "\t\t-l local file name\n");
	fprintf(stderr, "\t\t-n suggested number of threads\n");
	fprintf(stderr, "\t\t-f force using suggested number of threads\n");
	fprintf(stderr, "\t\t-h this screen\n");
	fprintf(stderr, "\t\t-v version info\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "http//www.enderunix.org/aget/\n");
}

/* reverse a given string	*/
void revstr(char *str)
{
	char *p, *s;
	int i;
	int size;

	if ((size = strlen(str)) == 0)
		return;
	p = (char *)calloc(size, sizeof(char));
	s = p;
	for (i = size; i >= 0; i--, s++)
		*s = *(str + i - 1);
	*s = '\0';
	strncpy(str, p, size);
	str[size] = '\0';
	free(p);
}

/* Log	*/
void Log(char *fmt, ...)
{
	va_list ap;
	char *lfmt;

	lfmt = (char *)calloc(7 + strlen(fmt), sizeof(char));
	sprintf(lfmt, "<LOG> %s", fmt);

	fflush(stdout);
	va_start(ap, fmt);
	vfprintf(stderr, lfmt, ap);
	va_end(ap);

	if (fmt[0] != '\0' && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s", strerror(errno));
	fprintf(stderr, "\n");
	free(lfmt);
}

/* Progress Bar	*/
void updateProgressBar(float cur, float tot)
{
	float rat;
	int ndot, i;
	static float prev = -1;

	rat = cur/tot;
	ndot = (int)(rat * 100);

	if ((ndot < prev + 5) && (ndot != 100))
		return;

	for (i = 0; i < ndot; i += 2)
		putchar('.');
	for (i = ndot - 1; i < 100; i += 2)
		putchar(' ');
	printf("[%d%% completed]\n", ndot);	
	prev = ndot;
}

void handleHttpRetcode(char *rbuf)
{

	if ((strstr(rbuf, "HTTP/1.1 416")) != NULL) {
		Log("Server returned HTTP/1.1 416 - Requested Range Not Satisfiable\n");
	        exit(1);
	} else
	if ((strstr(rbuf, "HTTP/1.1 403")) != NULL) {
		Log("<Server returned HTTP/1.1 403 - Permission Denied\n");
		exit(1);
	} else
	if ((strstr(rbuf, "HTTP/1.1 404")) != NULL) {
		Log("<Server returned HTTP/1.1 404 - File Not Found\n");
		exit(1);
	} 
}
