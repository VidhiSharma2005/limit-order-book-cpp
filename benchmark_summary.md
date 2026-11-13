# Limit Order Book — Benchmark Summary

**Status:** Preliminary performance baseline  
**Language:** C++17  
**Build system:** CMake + Ninja  
**Benchmark framework:** Google Benchmark  
**Latency measurement:** `std::chrono::steady_clock`

## 1. Test Environment

| Component           |                           Configuration |
|---------------------|-----------------------------------------|
| Laptop              |                          Samsung 960QFG |
| Processor           |            13th Gen Intel Core i7-1360P |
| Physical cores      |                                      12 |
| Logical processors  |                                      16 |
| Installed RAM       |                                15.57 GB |
| Operating system    | Windows 11 Home Single Language, 64-bit |
| Compiler            |               GCC 16.2.0 (MSYS2 UCRT64) |
| CMake               |                                   4.4.4 |
| Ninja               |                                  1.13.2 |
| C++ standard        |                                   C++17 |
| Build configuration |                                 Release |

## 2. Implementation Under Test

The current order book uses:

- `std::map` for ordered bid and ask price levels.
- `std::list` for FIFO order queues at each price level.
- `std::unordered_map` for order-ID lookup.

## 3. Benchmark Methodology

The benchmark suite measures insertion, matching, cancellation, and modification.

Workloads use 100, 1,000, and 10,000 resting orders.

### Google Benchmark

- Five repetitions per benchmark configuration.
- Setup and destruction are excluded from timed intervals where explicitly paused.
- Correctness checks are performed outside timed intervals.
- Throughput is reported as processed orders per second.

### Latency Benchmark

- 100 warm-up samples per workload.
- 1,000 measured samples per workload.
- A fresh order book is prepared for each sample.
- Setup and correctness verification are outside the measured interval.
- Reports p50, p95, and p99 latency.
- Uses `std::chrono::steady_clock`.

Adjacent clock-read calibration measured approximately 100 ns. This is a reference measurement and is not subtracted from operation latency.

## 4. Throughput Results

Preliminary Google Benchmark results:

| Operation | Side | 100 orders | 1,000 orders | 10,000 orders |
|-----------|------|------------|--------------|---------------|
| Insertion | BUY  |    2.93M/s |      3.08M/s |       2.77M/s |
| Insertion | SELL |    3.08M/s |      2.99M/s |       2.90M/s |
| Matching  | BUY  |    5.39M/s |      5.13M/s |       5.16M/s |
| Matching  | SELL |    5.18M/s |      5.61M/s |       5.20M/s |

Throughput values are approximate orders processed per second. Matching throughput counts resting orders matched by the incoming order.

## 5. Latency Results

Latency measurements at 10,000 resting orders:

| Operation          | Side | p50 (µs) | p95 (µs) | p99 (µs) |
|--------------------|------|----------|----------|----------|
| Add order          | BUY  |      1.2 |      2.2 |      2.5 |
| Add order          | SELL |      0.4 |      0.8 |      1.0 |
| Single-order match | BUY  |      2.3 |      4.6 |      5.5 |
| Single-order match | SELL |      2.0 |      4.7 |      5.4 |
| Bulk match         | BUY  |  1,578.3 |  2,704.3 |  3,587.7 |
| Bulk match         | SELL |  1,637.8 |  3,008.1 |  4,031.2 |
| Cancellation       | BUY  |      1.4 |      4.4 |      5.2 |
| Cancellation       | SELL |      0.9 |      4.5 |      5.3 |
| Quantity reduction | BUY  |      0.3 |      0.7 |      0.9 |
| Quantity reduction | SELL |      0.3 |      0.7 |      0.9 |
| Price replacement  | BUY  |      2.4 |      6.1 |      7.3 |
| Price replacement  | SELL |      2.0 |      5.4 |      6.2 |

Values are in microseconds. Bulk matching consumes all 10,000 resting orders and generates 10,000 trades. It is not directly comparable to a single-order match.

## 6. Workload Characteristics

The benchmark workloads include:

- Insertion into an order book with existing resting orders.
- A single incoming order matching one resting order.
- An incoming order consuming all resting orders in a bulk-match workload.
- Cancellation of an order in the middle of a price-level queue.
- Reduction of an existing order's remaining quantity.
- Replacement of an order after a price change.

The workloads exercise different order-book depths and both BUY and SELL sides.

## 7. Measurement Limitations

These results are a preliminary baseline, not final performance claims.

1. Google Benchmark results use five repetitions per configuration.
2. Latency measurements use 1,000 samples per workload; p99 can be sensitive to a small number of slow samples.
3. Measurements can vary with CPU scheduling, background processes, thermal conditions, and system load.
4. Operations with latency close to the approximately 100 ns clock-read interval require particular caution.
5. Bulk matching processes 10,000 orders and creates 10,000 trades, so it represents a different workload from single-order matching.
6. The current workloads do not yet cover a complete matrix of orders concentrated at a few price levels versus orders distributed across many price levels.
7. Results should be rerun under controlled conditions before being used as final resume metrics.

## 8. Next Benchmarking Milestone

Preserve this implementation as the `std::map` baseline.

The next experiment will compare alternative price-level data structures under identical workloads, order distributions, and correctness tests.

No performance improvement should be claimed until the alternative implementation has been measured against this baseline.