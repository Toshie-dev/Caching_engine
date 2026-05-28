#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include"./protocol/protocol.c"
#include "concurrency/event_loop.h"

// static void do_something(int connfd) {
//     char rbuf[64] = {};
//     
//     // read() returns 0 on EOF (client closed connection), or negative on error
//     ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
//     if (n < 0) {
//         perror("read() error");
//         return;
//     }
//     if (n == 0) {
//         printf("Client disconnected without sending data.\n");
//         return;
//     }
// 
//     // Ensure it's null-terminated before printing safety
//     rbuf[n] = '\0';
//     printf("client says: %s\n", rbuf);
// 
//     char wbuf[] = "world\n";
//     ssize_t bytes_written = write(connfd, wbuf, strlen(wbuf));
//     if (bytes_written < 0) {
//         perror("write() error");
//     }
// }

int start_listening_sock()
{
    // Create socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Set SO_REUSEADDR
    int val = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        perror("setsockopt() failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Configure address structure
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);        // Port 1234
    addr.sin_addr.s_addr = htonl(0);    // Wildcard IP 0.0.0.0 (INADDR_ANY)

    // Bind
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv < 0) {
        perror("bind() failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Listen (listening socket)
    rv = listen(fd, SOMAXCONN);
    if (rv < 0) {
        perror("listen() failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 1234...\n");

    return fd;
}


int main() {
    
    int lst_sock = start_listening_sock();
    
    start_event_loop(lst_sock);

    // Event Loop
   // while (true) {
   //     struct sockaddr_in client_addr = {};
   //     socklen_t addrlen = sizeof(client_addr);
   //     
   //     // connection socket
   //     int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
   //     if (connfd < 0) {
   //         perror("accept() error");
   //         continue;   // Skip this iteration and try again
   //     }
   //     // only serves one client connection at once
   //     while (true) {
   //         int32_t err = one_request(connfd);
   //         if (err) {
   //             break;
   //         }
   //     }
   //     close(connfd); // Clean up connection socket
   // }

    close(lst_sock); // Unreachable here, but good practice
    return 0;
}
