// Contains the main event loop, also responsible for handling new connecios

#include <vector>
#include <poll.h>
#include <cstdint>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <cassert>
#include "event_loop.h"
#include "../protocol/protocol.h"
#include "./handlers.c"

std::vector<Conn*> fd2conn;

void handle_read(Conn *conn);
void handle_write(Conn *conn);

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// set the socket non-blocking
static void fd_set_nb(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        die("fcntl F_GETFL failed");
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        die("fcntl F_SETFL failed");
    }
}

static Conn* handle_accept(int fd) {
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
    if (connfd < 0) {
        // If the resource is temporarily unavailable, simply skip
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return NULL; 
        }
        perror("accept() error");
        return NULL;
    }
    
    // set the new connection fd to nonblocking mode
    fd_set_nb(connfd);
    
    // create a new tracking instance
    Conn *conn = new Conn();
    conn->fd = connfd;
    conn->want_read = true; // wait for read operations
    return conn;
}

void start_event_loop(int lst_sock) {
    fd_set_nb(lst_sock);

    // the event loop
    std::vector<struct pollfd> poll_args;

    while (true) {
        poll_args.clear();
        
        // FIXED: Replaced undefined 'fd' with your parameterized input 'lst_sock'
        struct pollfd pfd_listen = {lst_sock, POLLIN, 0};
        poll_args.push_back(pfd_listen);
        
        // Map the existing connection configurations to application requirements
        for (Conn *conn : fd2conn) {
            if (!conn) {
                continue;
            }
            struct pollfd pfd = {conn->fd, POLLERR, 0};

            if (conn->want_read) {
                pfd.events |= POLLIN;
            }
            if (conn->want_write) {
                pfd.events |= POLLOUT;
            }
            poll_args.push_back(pfd);
        }

        // Synchronize on active file descriptors
        // main poll call
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), -1);
        if (rv < 0 && errno == EINTR) {
            continue;   // interrupted by signal, retry loop safely
        }
        if (rv < 0) {
            die("poll failed");
        }

        // handle incoming connection request events on the listening socket
        if (poll_args[0].revents & POLLIN) {
            printf("new connection\n");
            if (Conn *conn = handle_accept(lst_sock)) {
                // Resize vector array bounds to account for high file descriptors values
                if (fd2conn.size() <= (size_t)conn->fd) {
                    fd2conn.resize(conn->fd + 1, NULL);
                }
                fd2conn[conn->fd] = conn;
            }
        }

        // evaluate independent active network sockets connections
        for (size_t i = 1; i < poll_args.size(); ++i) { 
            uint32_t ready = poll_args[i].revents;
            int current_fd = poll_args[i].fd;
            
            Conn *conn = fd2conn[current_fd];
            if (!conn) continue; // Safety guard clause 

            if (ready & POLLIN) {
                handle_read(conn);  
            }
            // Check state again as logic within handle_read might request connection terminations
            if ((ready & POLLOUT) && !conn->want_close) {
                handle_write(conn); 
            }

            // Cleanup routine for connections dropped by peer or internal application state instructions
            if ((ready & POLLERR) || (ready & POLLHUP) || conn->want_close) {
                printf("connection closed\n");
                (void)close(conn->fd);
                fd2conn[conn->fd] = NULL;
                delete conn;
            }
        }
    }
}
