# Rate Limiter

A token-bucket rate limiter in C11.

## How it works

A bucket holds up to `capacity` tokens and refills at `refill_rate` tokens per second.
Each call to `bucket_allow` costs one token: if a token is available the call is allowed,
otherwise it is denied. A full bucket allows a burst of up to `capacity` calls; after that,
calls are limited to the refill rate.

`bucket_allow_n` charges `n` tokens instead, so requests can have different costs. It is
all-or-nothing: if fewer than `n` tokens are available, nothing is taken.

## Design choices

- **Integer milli-tokens.** Tokens are stored as `uint64_t` thousandths of a token, so
  fractional refills (e.g. 0.4 tokens after 400 ms at 1 token/s) are kept without floating point.
- **Caller-supplied time.** Functions take an absolute timestamp `now_ns` instead of reading
  the clock themselves. This keeps the logic deterministic and easy to test.
  `now_monotonic_ns()` wraps `CLOCK_MONOTONIC` for real use.
- **Input validation.** `bucket_init` returns `false` for a zero capacity or rate, or for values
  large enough to overflow the internal arithmetic. The maximum capacity is
  `UINT64_MAX / (1000 * NS_PER_SEC)`, about 18.4 million tokens.
- **Three-way result.** `bucket_allow_n` returns `BUCKET_DENIED` when the request may succeed
  later, and `BUCKET_INVALID` when it never can (`n == 0` or `n > capacity`). Neither takes tokens.
- **Backwards clock.** If `now_ns` is earlier than the last refill, elapsed time is treated as 0.
- **No lost time.** Only the time actually converted into tokens is consumed; the leftover
  carries over to the next call. Once the bucket is full, the timestamp resets to now.

## Build & test

Requires a C11 compiler with AddressSanitizer and UndefinedBehaviorSanitizer support
(gcc or clang on Linux).

```sh
make test    # build and run the tests
make clean   # remove build/
```

The build uses `-std=c11 -Wall -Wextra -Wconversion -Wpedantic` and runs the tests under
ASan + UBSan with `-fno-sanitize-recover=all`, so any memory error or undefined behavior fails the run.

The tests in `tests/test_bucket.c` cover bursts, fractional refills, leftover carry-over,
full-bucket reset, long idle periods, a backwards clock, fast refill rates, multi-token costs,
invalid requests, and init validation.

## Limitations

- One bucket per `bucket_t`; no keyed/per-client buckets.
- Not thread-safe: guard a shared bucket with a lock.
- `now_monotonic_ns()` uses POSIX `clock_gettime`.

## License

MIT. See [LICENSE](LICENSE).
