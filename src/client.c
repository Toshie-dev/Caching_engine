#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Fixed: Added the missing die helper function definition
void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    
    // Fixed: Changed ntohs to htons and ntohl to htonl
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // 127.0.0.1

    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv < 0) { // Good practice to explicitly check < 0 for socket errors
        die("connect");
    }

    char msg[] = "hello";
    write(fd, msg, strlen(msg));

    char rbuf[64] = {};
    ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        die("read");
    }
    
    // Safety check: Explicitly null-terminate before printing
    rbuf[n] = '\0';
    printf("server says: %s\n", rbuf);
    
    close(fd);
    return 0;
}
