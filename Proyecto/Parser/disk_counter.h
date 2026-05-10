#pragma once
#include <cstdint>

struct DiskCounter {
    static uint64_t reads;
    static uint64_t writes;

    static void reset() { reads = 0; writes = 0; }
    static uint64_t total() { return reads + writes; }
};