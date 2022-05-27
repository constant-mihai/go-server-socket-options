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

#define	SA	struct sockaddr

// The address can be returned in a sockaddr in case we would like to connect from outside.
// int
// udp_client(const char *host, const char *serv, SA **saptr, socklen_t *lenptr);
//*saptr = Malloc(res->ai_addrlen);
//memcpy(*saptr, res->ai_addr, res->ai_addrlen);
//*lenp = res->ai_addrlen;

int
udp_client(const char *host, const char *serv);

int
udp_server(const char *host, const char *serv, socklen_t *addrlenp);

#define	MAXLINE		4096	/* max text line length */

int		daemon_proc;		/* set nonzero by daemon_init() */

static void
err_log(int errnoflag, int level, const char *fmt, va_list ap)
{
	int		errno_save, n;
	char	buf[MAXLINE + 1];

	errno_save = errno;		/* value caller might want printed */
#ifdef	HAVE_VSNPRINTF
	vsnprintf(buf, MAXLINE, fmt, ap);	/* safe */
#else
	vsprintf(buf, fmt, ap);					/* not safe */
#endif
	n = strlen(buf);
	if (errnoflag)
		snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
	strcat(buf, "\n");

	if (daemon_proc) {
		syslog(level, fmt, ap);
	} else {
		fflush(stdout);		/* in case stdout and stderr are the same */
		fputs(buf, stderr);
		fflush(stderr);
	}
	return;
}

static void
err_quit(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_log(0, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}

// static void *
// Malloc(size_t size)
// {
//     void	*ptr;

//     if ( (ptr = malloc(size)) == NULL)
//         err_sys("malloc error");
//     return(ptr);
// }
#endif
