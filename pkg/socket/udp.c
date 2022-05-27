#include "udp.h"

static void
err_sys(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_log(1, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}

static void
Close(int fd)
{
	if (close(fd) == -1)
		err_sys("close error");
}

int
udp_client(const char *host, const char *serv)
{
	int				sockfd, n;
	struct addrinfo	hints, *res, *rp;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		err_quit("udp_client, error getting addr info for %s, %s: %s",
				 host, serv, gai_strerror(n));
	rp = res;

	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0) {
            fprintf(stderr, "udp_client; error opening socket\n");
			continue;
        }

        if (connect(sockfd, res->ai_addr, res->ai_addrlen) != -1) {
            fprintf(stdout, "udp_client; connected to socket\n");
            break;
        }

        Close(sockfd);
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL)
		err_sys("udp_client error connecting to %s, %s", host, serv);

	freeaddrinfo(rp);

	return(sockfd);
}

int
udp_server(const char *host, const char *serv, socklen_t *addrlenp)
{
	int				sockfd, n;
	struct addrinfo	hints, *res, *rp;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		err_quit("udp_server error for %s, %s: %s",
				 host, serv, gai_strerror(n));
	rp = res;

	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0)
			continue;		/* error - try next one */

		if (bind(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break;			/* success */

		Close(sockfd);		/* bind error - close and try next one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL)	/* errno from final socket() or bind() */
		err_sys("udp_server error for %s, %s", host, serv);

	if (addrlenp)
		*addrlenp = res->ai_addrlen;	/* return size of protocol address */

	freeaddrinfo(rp);

	return(sockfd);
}
