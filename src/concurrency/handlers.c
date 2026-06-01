#include "../protocol/protocol.h"
#include<iostream>
// Handlers for reading and writing to kernel stack and processing single requests as intitiated by the global event loop

const size_t k_max_msg_2 = 4096;

// append to the back
static void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}
// remove from the front
static void buf_consume(std::vector<uint8_t> &buf, size_t n) {
    buf.erase(buf.begin(), buf.begin() + n);
}

// process 1 request if there is enough data
static bool try_one_request(Conn *conn) {
    // 3. Try to parse the accumulated buffer.
    // Protocol: message header
    if (conn->incoming.size() < 4) {
        return false;   // want read
    }
    uint32_t len = 0;
    memcpy(&len, conn->incoming.data(), 4);
    if (len > k_max_msg_2) {  // protocol error
        conn->want_close = true;
        return false;   // want close
    }
    // Protocol: message body
    if (4 + len > conn->incoming.size()) {
        return false;   // want read
    }
    const uint8_t *request = &conn->incoming[4];
    
    // 4. Process the parsed message.
    std::vector<std::string> cmd;
    if (parse_req(request, (size_t)len, cmd) < 0) {
        conn->want_close = true;
        return false;   // error
    }

    Response resp;
    do_request(cmd, resp);
    // make_response(resp, conn->outgoing);
    // std::cout<<cmd<<std::endl;
    // generate the response (echo)
   // buf_append(conn->outgoing, (const uint8_t *)&len, 4);
   // buf_append(conn->outgoing, request, len);
   // // 5. Remove the message from `Conn::incoming`.
   // buf_consume(conn->incoming, 4 + len);
    return true;        // success
}

static void handle_read(Conn *conn) {
    // 1. Do a non-blocking read.
    uint8_t buf[64 * 1024];
    ssize_t rv = read(conn->fd, buf, sizeof(buf));
    if (rv <= 0) {  // handle IO error (rv < 0) or EOF (rv == 0)
        conn->want_close = true;
        return;
    }
    // 2. Add new data to the `Conn::incoming` buffer.
    buf_append(conn->incoming, buf, (size_t)rv);
    // 3. Try to parse the accumulated buffer.
    // 4. Process the parsed message.
    // 5. Remove the message from `Conn::incoming`.
    try_one_request(conn);
    // ...
    if (conn->outgoing.size() > 0) {    // has a response
        conn->want_read = false;
        conn->want_write = true;
    }   // else: want read

    if (conn->outgoing.size() == 0) {   // all data written
        conn->want_read = true;
        conn->want_write = false;
    } // else: want write


}

static void handle_write(Conn *conn) {
    assert(conn->outgoing.size() > 0);
    ssize_t rv = write(conn->fd, conn->outgoing.data(), conn->outgoing.size());
    if (rv < 0) {
        conn->want_close = true;    // error handling
        return;
    }
    // remove written data from `outgoing`
    buf_consume(conn->outgoing, (size_t)rv);
    // ...
    // update the readiness intention
    if (conn->outgoing.size() > 0) {    // has a response
        conn->want_read = false;
        conn->want_write = true;
    }   // else: want read

    if (conn->outgoing.size() == 0) {   // all data written
        conn->want_read = true;
        conn->want_write = false;
    } // else: want write
}
