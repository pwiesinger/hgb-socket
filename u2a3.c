//
// Created by Paul Wiesinger on 11.01.23.
//
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

/*
 *
 * Building and executing this client
 *
 * gcc -Wall u2a3.c -o client
 *
 * ./client localhost 4444
 *
 * */

#define BUF_SIZE 500

int main(int argc, char *argv[]) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int mainSocket, retVal;

    char buf[BUF_SIZE];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket stream = tcp, SOCK_DGRAM = udp */
    hints.ai_flags = AI_CANONNAME;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    retVal = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (retVal != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retVal));
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        mainSocket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (mainSocket < 0) {
            perror("socket");
            continue;
        }

        retVal = connect(mainSocket, rp->ai_addr, rp->ai_addrlen);

        if (retVal < 0) {
            perror("connect");
            close(mainSocket);
            continue;
        } else {
            // connection successful
            break;
        }
    }

    if (rp == NULL) {
        printf("Failed to connect to server\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);
    printf("Connected to server %s:%s\n", argv[1], argv[2]);

    while((retVal = read(mainSocket, buf, BUF_SIZE)) > 0) {
        write(1, buf, retVal);
        // read from console
        retVal = read(0, buf, BUF_SIZE);
        // write the buffer to the server
        write(mainSocket, buf, retVal);
    }

    close(mainSocket);
}