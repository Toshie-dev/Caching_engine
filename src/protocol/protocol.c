#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

const size_t k_max_msg = 4096;

// Added missing helper function for internal error logging
static void msg(const char *text) {
    fprintf(stderr, "%s\n", text);
}

// to read full data (TCP)
static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// to write full data (TCP)
static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// handles a single request from the client
static int32_t one_request(int connfd) {
    // 4 bytes header + max message body size
    char rbuf[4 + k_max_msg];
    errno = 0;
    
    // Read the 4-byte length header
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            // Clean EOF (Client disconnected gracefully)
            return err;
        } else {
            msg("read() header error");
            return err;
        }
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // Request body
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() body error");
        return err;
    }

    // Print client message safely
    printf("client says: %.*s\n", len, &rbuf[4]);

    // Reply using the same length-prefixed protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    
    return write_all(connfd, wbuf, 4 + len);
}
