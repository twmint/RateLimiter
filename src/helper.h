#ifndef HELPER_H
#define HELPER_H
#include <stdint.h>

static inline uint64_t min_u64(uint64_t a, uint64_t b) {
    return (a < b) ? a : b;
}

#endif
