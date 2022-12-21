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



extern void serve (int fd);

#define BUF_SIZE 500

int main(int argc, char *argv[]){
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int mainSocket, clientSocket, s;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
    char buf[BUF_SIZE];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
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

        serve(clientSocket);

        close(clientSocket);
    }
#pragma clang diagnostic pop


}

