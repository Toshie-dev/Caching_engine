#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <string>
#include <vector>

struct Response {
    uint32_t status = 0;
    std::vector<uint8_t> data;
};

void do_request(std::vector<std::string> &cmd, Response &out);

int32_t parse_req(const uint8_t *data, size_t size, std::vector<std::string> &out);
#endif
