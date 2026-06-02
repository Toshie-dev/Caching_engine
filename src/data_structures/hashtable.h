#include <cstddef>
#include <cstdint>
#include <string>
#include <cassert>
#include <cstdlib>
#include <vector>
#include "../protocol/protocol.h"


const size_t k_max_load_factor = 8;
const size_t k_rehashing_work = 128;    // constant work

// KV pair for the top-level hashtable

struct HNode {
    HNode *next = NULL;
    uint64_t hcode = 0; // hash value
};

struct HTab {
    HNode **tab = NULL; // array of slots
    size_t mask = 0;    // power of 2 array size, 2^n - 1
    size_t size = 0;    // number of keys
};

struct HMap {
    HTab newer;
    HTab older;
    size_t migrate_pos = 0;
};

struct Entry {
    struct HNode node;  // hashtable node
    std::string key;
    std::string val;
};

static struct {
    HMap db;    // top-level hashtable
} g_data;


struct LookupKey {  // for lookup only
    HNode node;
    std::string key;
};


HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
void   hm_insert(HMap *hmap, HNode *node);
HNode *hm_delete(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));


