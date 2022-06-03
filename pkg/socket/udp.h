#ifndef __UDP_IMPL_H__
#define __UDP_IMPL_H__

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>
#include <stdarg.h>
#include <errno.h>

#define SA  struct sockaddr
#define BUF_SIZE 500

// The address can be returned in a sockaddr in case we would like to connect from outside.
// int
// udp_client(const char *host, const char *serv, SA **saptr, socklen_t *lenptr);
//*saptr = Malloc(res->ai_addrlen);
//memcpy(*saptr, res->ai_addr, res->ai_addrlen);
//*lenp = res->ai_addrlen;
//
typedef struct {
    const char *host;
    const char *serv;
    socklen_t *addrlenp;

    int retval;
} server_t;

typedef struct {
    const char *host;
    const char *serv;
    socklen_t *addrlenp;
    SA **saptr;

    int retval;
} client_t;


int
udp_client(const char *host, const char *serv);

int
udp_read(int sockfd, char *buf);

void
udp_mread(int sockfd, char **buf);

void
udp_msend(int sockfd);

int
udp_socket(const char *host,
           const char *serv,
           struct sockaddr **saddr,
           socklen_t *addrlenp);

int
udp_server(const char *host, const char *serv, socklen_t *addrlenp);

#define MAXLINE     4096    /* max text line length */

int     daemon_proc;        /* set nonzero by daemon_init() */

// static void *
// Malloc(size_t size)
// {
//     void *ptr;

//     if ( (ptr = malloc(size)) == NULL)
//         err_sys("malloc error");
//     return(ptr);
// }
#endif
