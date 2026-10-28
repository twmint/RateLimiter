#ifndef BUCKET_H
#define BUCKET_H

#include <stdbool.h>
#include <stdint.h>

#define NS_PER_SEC 1000000000ULL // 1 second in nanoseconds

typedef struct {
    uint64_t capacity_mt;
    uint64_t refill_rate_mt;
    uint64_t tokens_mt;
    uint64_t last_refill_ns; // monotonic time, nanoseconds
} bucket_t;

typedef enum {
    BUCKET_ALLOWED,
    BUCKET_DENIED,
    BUCKET_INVALID
} bucket_result_t;

bool bucket_init(bucket_t *b, uint64_t capacity, uint64_t refill_rate, uint64_t now_ns);
bool bucket_allow(bucket_t *b, uint64_t now_ns);
bucket_result_t bucket_allow_n(bucket_t *b, uint64_t n, uint64_t now_ns);
uint64_t now_monotonic_ns(void);

#endif
