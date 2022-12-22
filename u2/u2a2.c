#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 500

extern void serve(int fd);
int clients =0;

int main(int argc, char *argv[]) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int mainSocket, clientSocket, s, yes;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
    char buf[BUF_SIZE];


    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    //IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_flags = AI_PASSIVE;    //use my IP
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    s = getaddrinfo(NULL, argv[1], &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        mainSocket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (mainSocket == -1) {
            //call socket() failed, try next configuration
            continue;
        }

        yes = 1;
        if (setsockopt(mainSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
            perror("setsockopt()");
            exit(EXIT_FAILURE);
        }

        if (bind(mainSocket, rp->ai_addr, rp->ai_addrlen) == 0) {
            // socket and bind were successful, we can stop creating sockets
            break;
        } else {
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
        s = accept(mainSocket, (struct sockaddr *) &clientSocket, &peer_addr_len);
        if (s < 0) {
            perror("accept");
            continue;
        }
        if (!fork()) {

            close(mainSocket); // the child doesn't need the listener
            clients++;  // increment the client count

            printf("Number of clients: %d\n", clients);
            serve(s);

            close(clientSocket);
            clients--;
            exit(0);
        }
        close(clientSocket);  // the parent doesn't need the new connection
    }
#pragma clang diagnostic pop
}
