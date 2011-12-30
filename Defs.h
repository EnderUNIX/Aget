#ifndef DEFS_H
#define DEFS_H


enum {
	GETREQSIZ = 256,
	GETRECVSIZ = 8192,
	HEADREQSIZ = 512,
	MAXURLSIZ = 1024,
	MAXHOSTSIZ = 1024,
	MAXIPSIZ = 16,
	MAXBUFSIZ = 512,
	MAXTHREADS = 25,
	HTTPPORT = 80,
	UNKNOWNREQ = 2,
	FTPREQ = 21,
	PROTO_HTTP = 0xFF,
	PROTO_FTP = 0x00,
	STAT_OK = 0xFF,		/* Download completed successfully	*/
	STAT_INT = 0x0F,	/* ^C caught, download interrupted	*/
	STAT_ERR = 0x00		/* Download finished with error		*/
};


#define	PROGVERSION  "EnderUNIX Aget v0.4.1"
#define	HEADREQ  "HEAD %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n"
#define	GETREQ  "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\nRange: bytes=%lld-\r\nConnection: close\r\n\r\n"

#endif
