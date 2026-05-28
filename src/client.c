#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  // For htons, htonl
#include <errno.h>
#include <stdint.h>
#include <assert.h>     // For assert()
#include "protocol/protocol.c"


static int32_t query(int fd, const char *text) {
    uint32_t len = (uint32_t)strlen(text);
    if (len > k_max_msg) {
        return -1;
    }

    // Send request
    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);  // assume little endian
    memcpy(&wbuf[4], text, len);

    // FIXED: Split variable declaration out of the 'if' statement condition
    int32_t err = write_all(fd, wbuf, 4 + len);
    if (err) {
        return err;
    }

    // 4 bytes header
    char rbuf[4 + k_max_msg];
    errno = 0;
    err = read_full(fd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    memcpy(&len, rbuf, 4);  // assume little endian
    printf("%d\n", len);
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // Reply body
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // Output server response safely
    printf("server says: %.*s\n", len, &rbuf[4]);
    return 0;
}

// Helper function to crash gracefully on initialization failures
void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main() {

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // 127.0.0.1

    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv < 0) { 
        die("connect");
    }

    // Send first query
    int32_t err = query(fd, "hello1");
    if (err) {
        goto L_DONE;
    }

    // Send second query over the same connection
    err = query(fd, "hello2");
    if (err) {
        goto L_DONE;
    }

L_DONE:
    close(fd);
    return 0;
}
