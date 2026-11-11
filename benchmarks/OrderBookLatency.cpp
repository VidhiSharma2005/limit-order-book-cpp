
#include "OrderBook.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;

constexpr int WARMUP_COUNT = 100;
constexpr int SAMPLE_COUNT = 1000;
constexpr int CLOCK_CALIBRATION_COUNT = 20000;

double percentile(
    const std::vector<double>& samples,
    double percentileValue
) {
    const auto index = static_cast<std::size_t>(
        std::ceil(percentileValue * samples.size())
    ) - 1;

    return samples[index];
}

void printPercentiles(
    const std::string& name,
    std::vector<double> samples
) {
    std::sort(samples.begin(), samples.end());

    std::cout
        << name
        << " | p50=" << percentile(samples, 0.50) << " ns"
        << " | p95=" << percentile(samples, 0.95) << " ns"
        << " | p99=" << percentile(samples, 0.99) << " ns"
        << '\n';
}

void calibrateClock() {
    std::vector<double> samples;
    samples.reserve(CLOCK_CALIBRATION_COUNT);

    for (int i = 0; i < CLOCK_CALIBRATION_COUNT; ++i) {
        const auto start = Clock::now();
        const auto end = Clock::now();

        samples.push_back(
            std::chrono::duration<double, std::nano>(
                end - start
            ).count()
        );
    }

    printPercentiles("Clock interval overhead", samples);
}

template <typename Setup, typename Operation, typename Verify>
void measureLatency(
    const std::string& name,
    int depth,
    Setup setup,
    Operation operation,
    Verify verify
) {
    std::vector<double> samples;
    samples.reserve(SAMPLE_COUNT);

    // Warm-up: execute the same workload without recording timings.
    for (int i = 0; i < WARMUP_COUNT; ++i) {
        OrderBook book;
        const int targetId = setup(book, depth);
        const int result = operation(book, targetId, depth);

        if (!verify(book, targetId, depth, result)) {
            std::cerr << "Warm-up correctness failure: "
                      << name << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    // Actual measurements.
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        OrderBook book;
        const int targetId = setup(book, depth);

        const auto start = Clock::now();

        const int result = operation(book, targetId, depth);

        const auto end = Clock::now();

        if (!verify(book, targetId, depth, result)) {
            std::cerr << "Correctness failure: "
                      << name
                      << ", depth=" << depth
                      << ", sample=" << i
                      << '\n';
            std::exit(EXIT_FAILURE);
        }

        samples.push_back(
            std::chrono::duration<double, std::nano>(
                end - start
            ).count()
        );
    }

    std::cout << name << " | depth=" << depth
              << " | samples=" << SAMPLE_COUNT << " | ";

    printPercentiles("", samples);
}

// ------------------------------------------------------------
// INSERTION
// ------------------------------------------------------------

void runInsertionLatency(OrderSide side, int depth) {
    const bool isBuy = side == OrderSide::BUY;

    measureLatency(
        isBuy ? "Add BUY" : "Add SELL",
        depth,

        [side](OrderBook& book, int count) {
            for (int i = 0; i < count; ++i) {
                const std::int64_t price =
                    side == OrderSide::BUY
                    ? 100000 - i
                    : 100000 + i;

                book.addOrder(side, price, 10);
            }
            return -1;
        },

        [side](OrderBook& book, int, int) {
            return book.addOrder(side, 200000, 10);
        },

        [side](const OrderBook& book, int, int, int result) {
            return result > 0
                   && book.hasOrder(result)
                   && book.validate();
        }
    );
}

// ------------------------------------------------------------
// SINGLE-ORDER MATCHING
// One incoming order consumes exactly one resting order.
// ------------------------------------------------------------

void runSingleMatchLatency(OrderSide incomingSide, int depth) {
    const bool isBuy = incomingSide == OrderSide::BUY;

    measureLatency(
        isBuy ? "Single match BUY" : "Single match SELL",
        depth,

        [incomingSide](OrderBook& book, int count) {
            for (int i = 0; i < count; ++i) {
                if (incomingSide == OrderSide::BUY) {
                    book.addOrder(
                        OrderSide::SELL,
                        10000 + i,
                        1
                    );
                } else {
                    book.addOrder(
                        OrderSide::BUY,
                        10000 - i,
                        1
                    );
                }
            }
            return -1;
        },

        [incomingSide](OrderBook& book, int, int count) {
            if (incomingSide == OrderSide::BUY) {
                return book.addOrder(
                    OrderSide::BUY,
                    10000 + count - 1,
                    1
                );
            }

            return book.addOrder(
                OrderSide::SELL,
                10000 - count + 1,
                1
            );
        },

        [](const OrderBook& book, int, int, int result) {
            return result > 0
                   && book.getTrades().size() == 1
                   && book.validate();
        }
    );
}

// ------------------------------------------------------------
// BULK MATCHING
// One incoming order consumes all resting orders.
// ------------------------------------------------------------

void runBulkMatchLatency(OrderSide incomingSide, int depth) {
    const bool isBuy = incomingSide == OrderSide::BUY;

    measureLatency(
        isBuy ? "Bulk match BUY" : "Bulk match SELL",
        depth,

        [incomingSide](OrderBook& book, int count) {
            for (int i = 0; i < count; ++i) {
                if (incomingSide == OrderSide::BUY) {
                    book.addOrder(
                        OrderSide::SELL,
                        10000 + i,
                        1
                    );
                } else {
                    book.addOrder(
                        OrderSide::BUY,
                        10000 - i,
                        1
                    );
                }
            }
            return -1;
        },

        [incomingSide](OrderBook& book, int, int count) {
            if (incomingSide == OrderSide::BUY) {
                return book.addOrder(
                    OrderSide::BUY,
                    10000 + count - 1,
                    count
                );
            }

            return book.addOrder(
                OrderSide::SELL,
                10000 - count + 1,
                count
            );
        },

        [](const OrderBook& book, int, int count, int result) {
            return result > 0
                   && book.getTrades().size() ==
                          static_cast<std::size_t>(count)
                   && book.validate();
        }
    );
}

// ------------------------------------------------------------
// CANCELLATION
// ------------------------------------------------------------

void runCancellationLatency(OrderSide side, int depth) {
    const bool isBuy = side == OrderSide::BUY;

    measureLatency(
        isBuy ? "Cancel BUY" : "Cancel SELL",
        depth,

        [side](OrderBook& book, int count) {
            for (int i = 0; i < count; ++i) {
                const std::int64_t price =
                    side == OrderSide::BUY
                    ? 100000 - i
                    : 100000 + i;

                book.addOrder(side, price, 10);
            }

            return count / 2 + 1;
        },

        [](OrderBook& book, int targetId, int) {
            return book.cancelOrder(targetId) ? 1 : -1;
        },

        [](const OrderBook& book, int targetId, int, int result) {
            return result == 1
                   && !book.hasOrder(targetId)
                   && book.validate();
        }
    );
}

// ------------------------------------------------------------
// QUANTITY REDUCTION
// ------------------------------------------------------------

void runQuantityReductionLatency(OrderSide side, int depth) {
    const bool isBuy = side == OrderSide::BUY;

    measureLatency(
        isBuy ? "Reduce BUY quantity" : "Reduce SELL quantity",
        depth,

        // 1. Setup: insert resting orders before timing.
        [side](OrderBook& book, int count) {
            for (int i = 0; i < count; ++i) {
                const std::int64_t price =
                    side == OrderSide::BUY
                    ? 100000 - i
                    : 100000 + i;

                book.addOrder(side, price, 10);
            }

            // Select the middle order for modification.
            return count / 2 + 1;
        },

        // 2. Operation: reduce the selected order's quantity from 10 to 5.
        [side](OrderBook& book, int targetId, int) {
            const std::int64_t price =
                side == OrderSide::BUY
                ? 100000 - (targetId - 1)
                : 100000 + (targetId - 1);

            return book.modifyOrder(
                targetId,
                price,
                5
            );
        },

        // 3. Verification: confirm the modification actually succeeded.
        [side](
            const OrderBook& book,
            int targetId,
            int,
            int result
        ) {
            const std::int64_t price =
                side == OrderSide::BUY
                ? 100000 - (targetId - 1)
                : 100000 + (targetId - 1);

            return result == targetId
                   && book.getQuantityAtPrice(side, price) == 5
                   && book.validate();
        }
    );
}

// ------------------------------------------------------------
// ORDER REPLACEMENT
// Price change creates a new order ID.
// ------------------------------------------------------------

void runReplacementLatency(OrderSide side, int depth) {
    const bool isBuy = side == OrderSide::BUY;

    measureLatency(
        isBuy ? "Replace BUY" : "Replace SELL",
        depth,

        [side](OrderBook& book, int count) {
            for (int i = 0; i < count; ++i) {
                const std::int64_t price =
                    side == OrderSide::BUY
                    ? 100000 - i
                    : 100000 + i;

                book.addOrder(side, price, 10);
            }

            return count / 2 + 1;
        },

        [side](OrderBook& book, int targetId, int) {
            return book.modifyOrder(
                targetId,
                200000,
                10
            );
        },

        [](const OrderBook& book, int targetId, int count, int result) {
            return result == count + 1
                   && !book.hasOrder(targetId)
                   && book.hasOrder(result)
                   && book.validate();
        }
    );
}

// ------------------------------------------------------------
// MAIN
// ------------------------------------------------------------

int main() {
    const std::vector<int> depths = {
        100,
        1000,
        10000
    };

    std::cout << "Limit Order Book Latency Results\n";
    std::cout << "Warm-up samples: " << WARMUP_COUNT << '\n';
    std::cout << "Measured samples: " << SAMPLE_COUNT << "\n\n";

    calibrateClock();

    for (int depth : depths) {
        std::cout << "\n========== "
                  << depth
                  << " RESTING ORDERS ==========\n";

        for (OrderSide side : {OrderSide::BUY, OrderSide::SELL}) {
            runInsertionLatency(side, depth);
            runSingleMatchLatency(side, depth);
            runBulkMatchLatency(side, depth);
            runCancellationLatency(side, depth);
            runQuantityReductionLatency(side, depth);
            runReplacementLatency(side, depth);
        }
    }

    return 0;
}
