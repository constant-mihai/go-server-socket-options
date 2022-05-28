#include "udp.h"
#include <pthread.h>

const char *stop_test_g = "stop test";

/*
 * Thread start routine.
 */
void *server_thread(void *arg)
{
    char buf[BUF_SIZE];
    struct sockaddr peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
    int sockfd, s;

    server_t *server = (server_t*)arg;
    sockfd = udp_server(server->host, server->serv, server->addrlenp);
    server->retval = 0;

    for (;;) {
        peer_addr_len = sizeof(struct sockaddr);
        nread = recvfrom(sockfd, buf, BUF_SIZE, 0,
            (struct sockaddr *) &peer_addr, &peer_addr_len);
        if (nread == -1) continue;		   /* Ignore failed request */

        char host[NI_MAXHOST], service[NI_MAXSERV];

        s = getnameinfo((struct sockaddr *) &peer_addr,
               peer_addr_len, host, NI_MAXHOST,
               service, NI_MAXSERV, NI_NUMERICSERV);

        if (s == 0) printf("Received %zd bytes from %s:%s\n", nread, host, service);
        else fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));

        if (!strncmp(buf, stop_test_g, strlen(stop_test_g))) {
            return arg;
        }

        if (sendto(sockfd, buf, nread, 0,
                   (struct sockaddr *) &peer_addr,
                   peer_addr_len) != nread) fprintf(stderr, "Error sending response\n");
    }

    return arg;
}

void *client_thread(void *arg)
{
    char buf[BUF_SIZE];
    ssize_t len, nread, nwrite;
    int sockfd;
    const char *message = "test string";

    client_t *client = (client_t*)arg;
    sockfd = udp_client(client->host, 
                        client->serv);

    for (int i=0; i<100; i++) {
        len = strlen(message) + 1;
        /* +1 for terminating null byte */

        nwrite = write(sockfd, message, len);
        if (nwrite != len) {
            fprintf(stderr, "partial/failed write; wrote %li bytes", nwrite);
            client->retval = -1;
            return arg;
        }

        nread = read(sockfd, buf, BUF_SIZE);
        if (nread == -1) {
            fprintf(stderr, "error reading");
            client->retval = -1;
            return arg;
        }

        printf("Received %zd bytes: %s\n", nread, buf);
    }
    len = strlen(message) + 1;
    /* +1 for terminating null byte */
    nwrite = write(sockfd, stop_test_g, len);
    if (nwrite != len) {
        fprintf(stderr, "partial/failed write; wrote %li bytes", nwrite);
        client->retval = -1;
        return arg;
    }


    return arg;
}

//int main(int argc, char **argv) {
//    daemon_proc = 0;
//    printf("main\n");
//    (void) argc;
//    (void) argv;
//    pthread_t thread_ids[2];
//    void *thread_result;
//    int status;
//
//    server_t s = {
//        .host = "0.0.0.0",
//        .serv = "12345",
//    };
//
//    client_t c = {
//        .host = "127.0.0.1",
//        .serv = "12345",
//    };
//    status = pthread_create ( &thread_ids[0], NULL, server_thread, (void*)&s);
//    if (status != 0) err_quit ("Server thread %d", strerror (status));
//
//    sleep(0.10);
//    status = pthread_create ( &thread_ids[1], NULL, client_thread, (void*)&c);
//    if (status != 0) err_quit ("Client thread %d", strerror (status));
//
//    status = pthread_join (thread_ids[1], &thread_result);
//    if (status != 0) err_quit ("Join thread %d", strerror(status));
//    client_t *client = (client_t*) thread_result;
//    printf("client retval: %i\n", client->retval);
//    fflush(stdout);		/* in case stdout and stderr are the same */
//
//    status = pthread_join (thread_ids[0], &thread_result);
//    if (status != 0) err_quit ("Join thread %d", strerror(status));
//    server_t *server = (server_t*) thread_result;
//    printf("server retval: %i", server->retval);
//
//    /* if (thread_result == NULL) return 0;*/
//    /* else return 1;*/
//
//    return 0;
//}
