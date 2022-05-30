#include "udp.h"

static void
err_log(int errnoflag, int level, const char *fmt, va_list ap)
{
    int     errno_save, n;
    char    buf[MAXLINE + 1];

    errno_save = errno;     /* value caller might want printed */
#ifdef  HAVE_VSNPRINTF
    vsnprintf(buf, MAXLINE, fmt, ap);   /* safe */
#else
    vsprintf(buf, fmt, ap);                 /* not safe */
#endif
    n = strlen(buf);
    if (errnoflag)
        snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
    strcat(buf, "\n");

    if (daemon_proc) {
        syslog(level, fmt, ap);
    } else {
        fflush(stdout);     /* in case stdout and stderr are the same */
        fputs(buf, stderr);
        fflush(stderr);
    }
    return;
}

static void
err_sys(const char *fmt, ...)
{
    va_list     ap;

    va_start(ap, fmt);
    err_log(1, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(1);
}

static void
err_quit(const char *fmt, ...)
{
    va_list     ap;

    va_start(ap, fmt);
    err_log(0, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(1);
}

static void
Close(int sockfd)
{
    if (close(sockfd) == -1)
        err_sys("close error");
}

udp_read(int sockfd, char *buf)
{
    struct sockaddr peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
    int s;

    //sockfd = udp_server(server->host, server->serv, server->addrlenp);

    peer_addr_len = sizeof(struct sockaddr);
    nread = recvfrom(sockfd, buf, BUF_SIZE, 0,
                     (struct sockaddr *) &peer_addr, &peer_addr_len);
    if (nread == -1) {
        fprintf(stderr, "error reading from socket");
        return nread;
    }

    char host[NI_MAXHOST], service[NI_MAXSERV];

    s = getnameinfo((struct sockaddr *) &peer_addr,
                    peer_addr_len, host, NI_MAXHOST,
                    service, NI_MAXSERV, NI_NUMERICSERV);

    if (s == 0) printf("Received %zd bytes from %s:%s\n", nread, host, service);
    else fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));


    if (sendto(sockfd, buf, nread, 0,
               (struct sockaddr *) &peer_addr,
               peer_addr_len) != nread) fprintf(stderr, "Error sending response\n");

    return nread;
}

int udp_msend(int sockfd)
{
    struct mmsghdr *messages = malloc(sizeof(struct mmsghdr) * BATCH_SIZE);
    memset(messages, 0, sizeof(messages));

    int i;
    for (i=0; i<BATCH_SIZE; i++) {
        struct iovec *iov = malloc(sizeof(struct iovec));
        iov[0].iov_base=hello;
        iov[0].iov_len=strlen(hello);
        messages[i].msg_hdr.msg_iov=iov;
        messages[i].msg_hdr.msg_iovlen=1;
    }

    struct timespec wait_time;
    wait_time.tv_sec = 0;
    wait_time.tv_nsec = 1000000/rate;

    int sent;
    while (1) {
        if ((sent = sendmmsg(sockfd, messages, BATCH_SIZE, 0)) == -1) {
            printf("%s\n", strerror(errno));
            exit(errno);
        }
        sendcnt += sent;
        nanosleep(&wait_time, NULL);
    }
    return NULL;
}

int
udp_mread(int sockfd, char *buf)
{
    struct mmsghdr *messages = malloc(sizeof(struct mmsghdr) * BATCH_SIZE);
    memset(messages, 0, sizeof(messages));

    int i;
    for (i=0; i<BATCH_SIZE; i++) {
        struct iovec *iov = malloc(sizeof(struct iovec));
        iov[0].iov_base=malloc(BUF_LEN * sizeof(char));;
        iov[0].iov_len=BUF_LEN;
        messages[i].msg_hdr.msg_iov=iov;
        messages[i].msg_hdr.msg_iovlen=1;
    }

    struct timespec timeout;
    timeout.tv_sec = 1;
    timeout.tv_nsec = 0;

    int cnt = 0;
    int n;
    while (1) {
        n = recvmmsg(sockfd, messages, BATCH_SIZE, 0, &timeout);
        for (i=0; i<n; i++) {
            if (messages[i].msg_len == strlen(hello) && memcmp(hello, messages[i].msg_hdr.msg_iov[0].iov_base, n) == 0) {
                if (++cnt == TEST_NUM) {
                    return;
                }
            }
        }
    }
}

int
udp_client(const char *host, const char *serv)
{
    int             sockfd, n;
    struct addrinfo hints, *res, *rp;

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
udp_socket(const char *host,
           const char *serv,
           struct sockaddr **saddr,
           socklen_t *addrlenp)
{
    int             sockfd, n;
    struct addrinfo hints, *res, *rp;

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
        if (sockfd < 0) {
            fprintf(stderr, "error opening socket %s\n", strerror(errno));
        } else {
            int optval = 1;
            // https://lwn.net/Articles/542629/
            // https://stackoverflow.com/questions/14388706/how-do-so-reuseaddr-and-so-reuseport-differ
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT,
                           (void*)&optval, sizeof(optval)) ==-1){
                fprintf(stderr, "error setting socket options:  %s\n", strerror(errno));
                /* try to continue */
            }
            break;
        }
    } while ( (res = res->ai_next) != NULL);

    if (addrlenp)
        *addrlenp = res->ai_addrlen;
    if (saddr)
        *saddr = res->ai_addr;

    freeaddrinfo(rp);

    return sockfd;
}

int
udp_server(const char *host, const char *serv, socklen_t *addrlenp)
{
    int             sockfd, n;
    struct addrinfo hints, *res, *rp;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
        err_quit("udp_server error for %s, %s: %s",
                 host, serv, gai_strerror(n));
    rp = res;

    do {
        //sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0)
            continue;       /* error - try next one */

        int optval = 1; // omfg, is this actually an enable/disable flag?
        // https://lwn.net/Articles/542629/
        // https://stackoverflow.com/questions/14388706/how-do-so-reuseaddr-and-so-reuseport-differ
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT,
                       (void*)&optval, sizeof(optval)) == -1){
            fprintf(stderr, "error setting socket options:  %s\n", strerror(errno));
            /* try to continue */
        }

        if (bind(sockfd, res->ai_addr, res->ai_addrlen) == 0) {
            break;
        } else {
            fprintf(stderr, "error binding to socket: %s\n", strerror(errno));
        }

        Close(sockfd);      /* bind error - close and try next one */
    } while ( (res = res->ai_next) != NULL);

    if (res == NULL)    /* errno from final socket() or bind() */
        err_sys("udp_server error for %s, %s", host, serv);

    if (addrlenp)
        *addrlenp = res->ai_addrlen;    /* return size of protocol address */

    freeaddrinfo(rp);

    return(sockfd);
}
