#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include "bucket.h"
#include "helper.h"

uint64_t now_monotonic_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * NS_PER_SEC + (uint64_t)ts.tv_nsec;
}

bool bucket_init(bucket_t *b, uint64_t capacity, uint64_t refill_rate, uint64_t now_ns) {
    if (capacity == 0 || refill_rate == 0 || capacity > UINT64_MAX / (1000 * NS_PER_SEC) || refill_rate > UINT64_MAX / 1000) {
        return false;
    }

    b->capacity_mt       = capacity * 1000;
    b->refill_rate_mt    = refill_rate * 1000;
    b->tokens_mt         = capacity * 1000;
    b->last_refill_ns = now_ns;

    return true;
}

bool bucket_allow(bucket_t *b, uint64_t now_ns) {
    uint64_t elapsed_ns;
    if (now_ns <= b->last_refill_ns) {
        elapsed_ns = 0;
    } else {
        elapsed_ns = now_ns - b->last_refill_ns;
        uint64_t reach_cap_ns = b->capacity_mt * NS_PER_SEC / b->refill_rate_mt;
        if (elapsed_ns >= reach_cap_ns) {
            b->tokens_mt = b->capacity_mt;
            b->last_refill_ns = now_ns;
        } else {    
            uint64_t refill = elapsed_ns * b->refill_rate_mt / NS_PER_SEC;
            if (refill > 0) {
                b->tokens_mt = min_u64(b->tokens_mt + refill, b->capacity_mt);
                if (b->tokens_mt == b->capacity_mt) {
                    b->last_refill_ns = now_ns;
                } else {
                    b->last_refill_ns += refill * NS_PER_SEC / b->refill_rate_mt ;
                }
            }
        }
        
    }
    
    if (b->tokens_mt >= 1000) {
        b->tokens_mt -= 1000;
        return true;
    }
    return false;
}
