//
// Created by Paul Wiesinger on 21.12.22.
//

// gcc u1a2.c WordCheck.c

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>


extern void serve (int fd);

#define BUF_SIZE 500

char * get_ip_str ( const struct sockaddr *sa , char *s, size_t maxlen )
{
    switch (sa -> sa_family ) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *) sa) -> sin_addr), s,maxlen);
            break ;
        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *) sa) -> sin6_addr), s, maxlen);
            break ;
        default :
            strncpy (s, " Unknown AF", maxlen );
    }
    return s;
}

int get_port ( const struct sockaddr *sa)
{
    int port;
    switch (sa -> sa_family ) {
        case AF_INET:
            port = ntohs(((struct sockaddr_in *)sa) -> sin_port);
            break ;
        case AF_INET6:
            port = ntohs(((struct sockaddr_in6 *)sa) -> sin6_port);
            break ;
        default:
            port = -1;
    }
    return port;
}

int clientCount;

static void handler_sigchld (int signum) {
    int child_process_id, status, saved_errno;
    // save errno, wait () will change t
    saved_errno = errno;
    // will not block because is called within the signal handler for SIGCHLD
    child_process_id = wait (&status);

    clientCount--;
    printf("Child process finished, now serving %d clients\n", clientCount);

    // restore errno
    errno = saved_errno;
}

int main(int argc, char *argv[]){
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int mainSocket, clientSocket, s, yes, retVal;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
    char buf[BUF_SIZE];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sa.sa_handler = handler_sigchld;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, & sa, NULL) == -1) {
        perror ("sigaction");
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    s = getaddrinfo(NULL, argv[1], &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }
    for(rp = result; rp != NULL; rp = rp->ai_next){
        mainSocket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if(mainSocket == -1){
            //call socket() failed, try next configuration
            continue;
        }

        yes = 1;
        if (setsockopt(mainSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
            perror("setsockopt()");
            exit(EXIT_FAILURE);
        }

        if(bind(mainSocket, rp->ai_addr, rp->ai_addrlen)==0){
            // socket and bind were successful, we can stop creating sockets
            break;
        }else{
            close(mainSocket);
        }
    }

    freeaddrinfo(result);

    if (rp == NULL) {
        perror("socket() and bind()");
        exit(EXIT_FAILURE);
    }

    if (listen(mainSocket, 10)) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"



    for(;;) {
        peer_addr_len = sizeof(peer_addr);
        clientSocket = accept(mainSocket, (struct sockaddr *) &peer_addr, &peer_addr_len);
        if (clientSocket < 0) {
            // error rat accept()
            continue;
        }

        clientCount++;

        printf("Accepted new client nr %d from %s:%d via socket %d\n",
               clientCount,
               get_ip_str((struct sockaddr *) &peer_addr, buf, sizeof(buf)),
               get_port((struct sockaddr *) &peer_addr),
               clientSocket);

        retVal = fork();

        if (retVal < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (retVal == 0) {
            // child process
            serve(clientSocket);
            close(clientSocket);

            printf("closing socket from %s:%d %d\n",
                   get_ip_str((struct sockaddr *) &peer_addr, buf, sizeof(buf)),
                   get_port((struct sockaddr *) &peer_addr),
                   clientSocket);

            exit(EXIT_SUCCESS);
        } else {
            // parent process
            close(clientSocket);
        }
    }
#pragma clang diagnostic pop


}

