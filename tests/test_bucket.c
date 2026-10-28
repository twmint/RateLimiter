#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "bucket.h"

// Calls bucket_allow at time t until it denies; returns how many were allowed.
static int allow_all(bucket_t *b, uint64_t t) {
    int allowed = 0;
    while (bucket_allow(b, t)) allowed++;
    return allowed;
}

static void test_burst_then_deny(void) {
    bucket_t b;
    assert(bucket_init(&b, 5, 1, 0));
    for (int i = 0; i < 5; i++) assert(bucket_allow(&b, 0));
    assert(!bucket_allow(&b, 0));
}

static void test_frequent_calls_still_refills(void) {
    bucket_t b;
    assert(bucket_init(&b, 1, 1, 0));
    assert(bucket_allow(&b, 0));

    int allowed = 0;
    for (uint64_t t = 500000; t <= 2000000000ULL; t += 500000)   // every 0.5 ms for 2 s
        allowed += bucket_allow(&b, t);
    assert(allowed == 2);
}

static void test_one_token_per_second(void) {
    bucket_t b;
    assert(bucket_init(&b, 5, 1, 0));
    for (int i = 0; i < 5; i++) assert(bucket_allow(&b, 0));
    assert(!bucket_allow(&b, 1));
    assert(bucket_allow(&b, NS_PER_SEC));
    assert(!bucket_allow(&b, NS_PER_SEC));
}

static void test_leftover_fraction_kept(void) {
    bucket_t b;
    assert(bucket_init(&b, 5, 1, 0));
    for (int i = 0; i < 5; i++) assert(bucket_allow(&b, 0));

    int allowed = 0;
    uint64_t t = 1500000;
    for (int i = 0; i < 2000; i++) {
        allowed += bucket_allow(&b, t);
        t += 1500000;
    }
    assert(allowed == 3);
}

static void test_full_bucket_reset(void) {
    bucket_t b;
    assert(bucket_init(&b, 1, 1, 0));
    assert(bucket_allow(&b, 0));
    assert(bucket_allow(&b, 10 * NS_PER_SEC + NS_PER_SEC / 2));
    assert(!bucket_allow(&b, 10 * NS_PER_SEC + NS_PER_SEC / 2));
    assert(!bucket_allow(&b, 11 * NS_PER_SEC));
    assert(bucket_allow(&b, 11 * NS_PER_SEC + NS_PER_SEC / 2));
}

static void test_fractions_accumulate(void) {
    bucket_t b;
    assert(bucket_init(&b, 5, 1, 0));
    assert(allow_all(&b, 0) == 5);
    assert(!bucket_allow(&b, NS_PER_SEC / 2));   // 0.5 token so far
    assert(bucket_allow(&b, NS_PER_SEC));        // + 0.5 = 1 token
    assert(!bucket_allow(&b, NS_PER_SEC));
}

// Long idle, rate that doesn't divide evenly: reach_cap_ns rounds down, bucket must still fill
static void test_long_idle_uneven_rate(void) {
    bucket_t b;
    assert(bucket_init(&b, 1, 3, 0));
    assert(allow_all(&b, 0) == 1);
    assert(bucket_allow(&b, 3600 * NS_PER_SEC));
    assert(!bucket_allow(&b, 3600 * NS_PER_SEC));
}

static void test_idle_does_not_overfill(void) {
    bucket_t b;
    assert(bucket_init(&b, 5, 1, 0));
    assert(allow_all(&b, 10 * NS_PER_SEC) == 5);
}

static void test_backwards_clock(void) {
    bucket_t b;
    assert(bucket_init(&b, 1, 1, 0));
    assert(bucket_allow(&b, 10 * NS_PER_SEC));
    assert(!bucket_allow(&b, 9 * NS_PER_SEC));   // clock jumped back
    assert(!bucket_allow(&b, 10 * NS_PER_SEC));  // back to 10 s: no free token
    assert(bucket_allow(&b, 11 * NS_PER_SEC));
}

static void test_fast_rate(void) {
    bucket_t b;
    assert(bucket_init(&b, 10, 100, 0));
    assert(allow_all(&b, 0) == 10);
    assert(allow_all(&b, NS_PER_SEC / 10) == 10);   // 0.1 s at 100/s refills all 10
}

static void test_init_rejects_bad_input(void) {
    bucket_t b;
    uint64_t max_capacity = UINT64_MAX / (1000 * NS_PER_SEC);
    assert(!bucket_init(&b, 0, 1, 0));
    assert(!bucket_init(&b, 1, 0, 0));
    assert(!bucket_init(&b, max_capacity + 1, 1, 0));
    assert(bucket_init(&b, max_capacity, 1, 0));
}

static void test_multiple_token_per_req(void) {
    bucket_t b;
    assert(bucket_init(&b, 6, 2, 0));
    assert(bucket_allow_n(&b, 6, 0) == BUCKET_ALLOWED);
    assert(bucket_allow_n(&b, 1, 0) == BUCKET_DENIED);
    assert(bucket_allow_n(&b, 2, 1 * NS_PER_SEC) == BUCKET_ALLOWED);
    assert(bucket_allow_n(&b, 0, 1 * NS_PER_SEC) == BUCKET_INVALID);
    assert(bucket_allow_n(&b, 1, 3 * NS_PER_SEC / 2) == BUCKET_ALLOWED); // 1.5 s
    assert(bucket_allow_n(&b, 2, 2 * NS_PER_SEC) == BUCKET_DENIED); // a denied request takes nothing
    assert(bucket_allow_n(&b, 2, 5 * NS_PER_SEC / 2) == BUCKET_ALLOWED); // 2.5 s
}

static void test_req_token_exceeds_cap(void) {
    bucket_t b;
    assert(bucket_init(&b, 6, 2, 0));
    assert(bucket_allow_n(&b, 7, 0) == BUCKET_INVALID);
    assert(bucket_allow_n(&b, 6, 0) == BUCKET_ALLOWED);
}

int main(void) {
    test_burst_then_deny();
    test_frequent_calls_still_refills();
    test_leftover_fraction_kept();
    test_one_token_per_second ();
    test_full_bucket_reset();
    test_fractions_accumulate();
    test_long_idle_uneven_rate();
    test_idle_does_not_overfill();
    test_backwards_clock();
    test_fast_rate();
    test_init_rejects_bad_input();

    test_multiple_token_per_req();
    test_req_token_exceeds_cap();
    puts("all tests passed");
    return 0;
}
