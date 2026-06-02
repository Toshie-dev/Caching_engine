#define RES_NX -1
#define RES_ERR 0

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <map>
#include <string>
#include "./protocol.h"
#include "../data_structures/hashtable.h"

bool read_u32(const uint8_t *&cur, const uint8_t *end, uint32_t &out) {
    if (cur + 4 > end) {
        return false;
    }
    memcpy(&out, cur, 4);
    cur += 4;
    return true;
}

// placeholder; implemented later
// static std::map<std::string, std::string> g_data;

void do_request(std::vector<std::string> &cmd, Response &out) {
//    if (cmd.size() == 2 && cmd[0] == "get") {
//        auto it = g_data.find(cmd[1]);
//        if (it == g_data.end()) {
//            out.status = RES_NX;    // not found
//            return;
//        }
//        const std::string &val = it->second;
//        out.data.assign(val.begin(), val.end());
//    } else if (cmd.size() == 3 && cmd[0] == "set") {
//        g_data[cmd[1]].swap(cmd[2]);
//    } else if (cmd.size() == 2 && cmd[0] == "del") {
//        g_data.erase(cmd[1]);
//    } else {
//        out.status = RES_ERR;       // unrecognized command
//    }
}

bool read_str(const uint8_t *&cur, const uint8_t *end, size_t n, std::string &out) {
    if (cur + n > end) {
        return false;
    }
    out.assign(cur, cur + n);
    cur += n;
    return true;
}

int32_t parse_req(const uint8_t *data, size_t size, std::vector<std::string> &out) {
    const uint8_t *end = data + size;
    uint32_t nstr = 0;
    if (!read_u32(data, end, nstr)) {
        return -1;
    }
    if (nstr > 4096) {
        return -1;  // safety limit
    }

    while (out.size() < nstr) {
        uint32_t len = 0;
        if (!read_u32(data, end, len)) {
            return -1;
        }
        out.push_back(std::string());
        if (!read_str(data, end, len, out.back())) {
            return -1;
        }
    }
    if (data != end) {
        return -1;  // trailing garbage
    }
    return 0;
}
