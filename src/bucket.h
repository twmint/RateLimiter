#ifndef BUCKET_H
#define BUCKET_H
#define NS_PER_SEC 1000000000ULL // 1 second in nanoseconds

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint64_t capacity_mt;
    uint64_t refill_rate_mt;
    uint64_t tokens_mt;
    uint64_t last_refill_ns; // monotonic time, nanoseconds
} bucket_t;

bool bucket_init(bucket_t *b, uint64_t capacity, uint64_t refill_rate, uint64_t now_ns);
bool bucket_allow(bucket_t *b, uint64_t now_ns);
uint64_t now_monotonic_ns(void);

#endif
