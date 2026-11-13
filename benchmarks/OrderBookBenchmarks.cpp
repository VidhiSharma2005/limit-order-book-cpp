
#include "OrderBook.h"
#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>
#include <vector>


static void BM_AddBuyOrders(benchmark::State& state) {
    const int orderCount = static_cast<int>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        auto book = std::make_unique<OrderBook>();

        state.ResumeTiming();

        for (int i = 0; i < orderCount; ++i) {
            int64_t price = 100000 - i;

            int id = book->addOrder(
                OrderSide::BUY,
                price,
                10
            );

            benchmark::DoNotOptimize(id);
        }

        benchmark::ClobberMemory();

        state.PauseTiming();

        if (!book->validate()) {
            state.SkipWithError(
                "BUY insertion benchmark correctness check failed"
            );
            book.reset();
            break;
        }

        book.reset();

        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        state.iterations() * orderCount
    );
}

static void BM_AddSellOrders(benchmark::State& state) {
    const int orderCount = static_cast<int>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        auto book = std::make_unique<OrderBook>();

        state.ResumeTiming();

        for (int i = 0; i < orderCount; ++i) {
            int64_t price = 100000 + i;

            int id = book->addOrder(
                OrderSide::SELL,
                price,
                10
            );

            benchmark::DoNotOptimize(id);
        }

        benchmark::ClobberMemory();

        state.PauseTiming();

        if (!book->validate()) {
            state.SkipWithError(
                "SELL insertion benchmark correctness check failed"
            );
            book.reset();
            break;
        }

        book.reset();

        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        state.iterations() * orderCount
    );
}


static void BM_MatchBuyAgainstAsks(benchmark::State& state) {
    const int orderCount = static_cast<int>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        auto book = std::make_unique<OrderBook>();

        // Prepare resting SELL orders outside timed section.
        for (int i = 0; i < orderCount; ++i) {
            book->addOrder(
                OrderSide::SELL,
                10000 + i,
                1
            );
        }

        state.ResumeTiming();

        // Measure the incoming BUY order and its matching.
        int incomingId = book->addOrder(
            OrderSide::BUY,
            10000 + orderCount - 1,
            orderCount
        );

        benchmark::DoNotOptimize(incomingId);
        benchmark::DoNotOptimize(book->getTrades().size());
        benchmark::ClobberMemory();

        state.PauseTiming();

        if (incomingId <= 0 ||
            book->getTrades().size() !=
                static_cast<std::size_t>(orderCount) ||
            !book->validate()) {

            state.SkipWithError(
                "BUY matching benchmark correctness check failed"
            );

            book.reset();
            break;
        }

        book.reset();

        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        state.iterations() * orderCount
    );
}

static void BM_MatchSellAgainstBids(benchmark::State& state) {
    const int orderCount = static_cast<int>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        auto book = std::make_unique<OrderBook>();

        // Prepare resting BUY orders outside timed section.
        for (int i = 0; i < orderCount; ++i) {
            book->addOrder(
                OrderSide::BUY,
                10000 - i,
                1
            );
        }

        state.ResumeTiming();

        // Measure the incoming SELL order and its matching.
        int incomingId = book->addOrder(
            OrderSide::SELL,
            10000 - orderCount + 1,
            orderCount
        );

        benchmark::DoNotOptimize(incomingId);
        benchmark::DoNotOptimize(book->getTrades().size());
        benchmark::ClobberMemory();

        state.PauseTiming();

        if (incomingId <= 0 ||
            book->getTrades().size() !=
                static_cast<std::size_t>(orderCount) ||
            !book->validate()) {

            state.SkipWithError(
                "SELL matching benchmark correctness check failed"
            );

            book.reset();
            break;
        }

        book.reset();

        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        state.iterations() * orderCount
    );
}

// Benchmark: cancel N resting BUY orders.
static void BM_CancelBuyOrders(benchmark::State& state) {
    const int orderCount = static_cast<int>(state.range(0));

    std::vector<int> orderIds;
    orderIds.reserve(orderCount);

    for (auto _ : state) {
        state.PauseTiming();

        auto book = std::make_unique<OrderBook>();
        orderIds.clear();

        // Prepare the book outside the timed section.
        for (int i = 0; i < orderCount; ++i) {
            int id = book->addOrder(
                OrderSide::BUY,
                100000 - i,
                10
            );

            orderIds.push_back(id);
        }

        state.ResumeTiming();

        // Measure cancellation only.
        for (int id : orderIds) {
            benchmark::DoNotOptimize(book->cancelOrder(id));
        }

        benchmark::ClobberMemory();

        // Exclude destruction and cleanup from measurement.
        state.PauseTiming();
        book.reset();
        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        state.iterations() * orderCount
    );
}

// Benchmark: cancel N resting SELL orders.
static void BM_CancelSellOrders(benchmark::State& state) {
    const int orderCount = static_cast<int>(state.range(0));

    std::vector<int> orderIds;
    orderIds.reserve(orderCount);

    for (auto _ : state) {
        state.PauseTiming();

        auto book = std::make_unique<OrderBook>();
        orderIds.clear();

        // Prepare the book outside the timed section.
        for (int i = 0; i < orderCount; ++i) {
            int id = book->addOrder(
                OrderSide::SELL,
                100000 + i,
                10
            );

            orderIds.push_back(id);
        }

        state.ResumeTiming();

        // Measure cancellation only.
        for (int id : orderIds) {
            benchmark::DoNotOptimize(book->cancelOrder(id));
        }

        benchmark::ClobberMemory();

        // Exclude destruction and cleanup from measurement.
        state.PauseTiming();
        book.reset();
        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        state.iterations() * orderCount
    );
}



static void BM_ReduceBuyOrderQuantities(benchmark::State& state) {
    const int orderCount = static_cast<int>(state.range(0));

    std::vector<int> orderIds;
    orderIds.reserve(orderCount);

    for (auto _ : state) {
        state.PauseTiming();

        auto book = std::make_unique<OrderBook>();
        orderIds.clear();

        for (int i = 0; i < orderCount; ++i) {
            int id = book->addOrder(
                OrderSide::BUY,
                100000 - i,
                10
            );

            orderIds.push_back(id);
        }

        state.ResumeTiming();

        bool allSucceeded = true;

        for (int id : orderIds) {
            int result = book->modifyOrder(
                id,
                100000 - (id - 1),
                5
            );

            benchmark::DoNotOptimize(result);
            allSucceeded = allSucceeded && (result == id);
        }

        benchmark::ClobberMemory();

        state.PauseTiming();

        if (!allSucceeded || !book->validate()) {
            state.SkipWithError(
                "BUY quantity reduction correctness check failed"
            );
            book.reset();
            break;
        }

        book.reset();

        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        state.iterations() * orderCount
    );
}


static void BM_ReduceSellOrderQuantities(benchmark::State& state) {
    const int orderCount = static_cast<int>(state.range(0));

    std::vector<int> orderIds;
    orderIds.reserve(orderCount);

    for (auto _ : state) {
        state.PauseTiming();

        auto book = std::make_unique<OrderBook>();
        orderIds.clear();

        for (int i = 0; i < orderCount; ++i) {
            int id = book->addOrder(
                OrderSide::SELL,
                100000 + i,
                10
            );

            orderIds.push_back(id);
        }

        state.ResumeTiming();

        bool allSucceeded = true;

        for (int id : orderIds) {
            int result = book->modifyOrder(
                id,
                100000 + (id - 1),
                5
            );

            benchmark::DoNotOptimize(result);
            allSucceeded = allSucceeded && (result == id);
        }

        benchmark::ClobberMemory();

        state.PauseTiming();

        if (!allSucceeded || !book->validate()) {
            state.SkipWithError(
                "SELL quantity reduction correctness check failed"
            );
            book.reset();
            break;
        }

        book.reset();

        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        state.iterations() * orderCount
    );
}


static void BM_ReplaceBuyOrders(benchmark::State& state) {
    const int orderCount = static_cast<int>(state.range(0));

    std::vector<int> orderIds;
    orderIds.reserve(orderCount);

    for (auto _ : state) {
        state.PauseTiming();

        auto book = std::make_unique<OrderBook>();
        orderIds.clear();

        for (int i = 0; i < orderCount; ++i) {
            int id = book->addOrder(
                OrderSide::BUY,
                100000 - i,
                10
            );

            orderIds.push_back(id);
        }

        state.ResumeTiming();

        bool allSucceeded = true;

        for (int i = 0; i < orderCount; ++i) {
            int result = book->modifyOrder(
                orderIds[i],
                90000 - i,
                10
            );

            const int expectedNewId = orderCount + i + 1;

            benchmark::DoNotOptimize(result);
            allSucceeded =
                allSucceeded && (result == expectedNewId);
        }

        benchmark::ClobberMemory();

        state.PauseTiming();

        if (!allSucceeded || !book->validate()) {
            state.SkipWithError(
                "BUY price replacement correctness check failed"
            );
            book.reset();
            break;
        }

        book.reset();

        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        state.iterations() * orderCount
    );
}


static void BM_ReplaceSellOrders(benchmark::State& state) {
    const int orderCount = static_cast<int>(state.range(0));

    std::vector<int> orderIds;
    orderIds.reserve(orderCount);

    for (auto _ : state) {
        state.PauseTiming();

        auto book = std::make_unique<OrderBook>();
        orderIds.clear();

        for (int i = 0; i < orderCount; ++i) {
            int id = book->addOrder(
                OrderSide::SELL,
                100000 + i,
                10
            );

            orderIds.push_back(id);
        }

        state.ResumeTiming();

        bool allSucceeded = true;

        for (int i = 0; i < orderCount; ++i) {
            int result = book->modifyOrder(
                orderIds[i],
                110000 + i,
                10
            );

            const int expectedNewId = orderCount + i + 1;

            benchmark::DoNotOptimize(result);
            allSucceeded =
                allSucceeded && (result == expectedNewId);
        }

        benchmark::ClobberMemory();

        state.PauseTiming();

        if (!allSucceeded || !book->validate()) {
            state.SkipWithError(
                "SELL price replacement correctness check failed"
            );
            book.reset();
            break;
        }

        book.reset();

        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        state.iterations() * orderCount
    );
}


// ============================================================
// Controlled price-level distribution benchmarks
// Fixed order count: 10,000
// Variable price levels: 100, 1,000, 10,000
// ============================================================

static constexpr int DISTRIBUTED_ORDER_COUNT = 10000;

// Returns a non-crossing price for an order on the given side.
// Multiple orders intentionally share the same price.
static int64_t distributedPrice(
    OrderSide side,
    int index,
    int priceLevelCount
) {
    const int level = index % priceLevelCount;

    if (side == OrderSide::BUY) {
        return 100000 - level;
    }

    return 100000 + level;
}

// Replacement prices are kept far away from original prices
// so replacements do not accidentally cross other orders.
static int64_t replacementPrice(
    OrderSide side,
    int index,
    int priceLevelCount
) {
    const int level = index % priceLevelCount;

    if (side == OrderSide::BUY) {
        return 50000 - level;
    }

    return 150000 + level;
}

static bool validDistributionArguments(
    benchmark::State& state,
    int orderCount,
    int priceLevelCount
) {
    if (orderCount != DISTRIBUTED_ORDER_COUNT ||
        priceLevelCount <= 0 ||
        priceLevelCount > orderCount ||
        orderCount % priceLevelCount != 0) {
        state.SkipWithError(
            "Invalid distributed benchmark arguments"
        );
        return false;
    }

    return true;
}

// ------------------------------------------------------------
// 1. Distributed insertion
// ------------------------------------------------------------

static void BM_DistributedAddOrders(
    benchmark::State& state,
    OrderSide side
) {
    const int orderCount = static_cast<int>(state.range(0));
    const int priceLevelCount = static_cast<int>(state.range(1));

    if (!validDistributionArguments(
            state, orderCount, priceLevelCount)) {
        return;
    }

    for (auto _ : state) {
        state.PauseTiming();

        auto book = std::make_unique<OrderBook>();

        state.ResumeTiming();

        for (int i = 0; i < orderCount; ++i) {
            int id = book->addOrder(
                side,
                distributedPrice(side, i, priceLevelCount),
                10
            );

            benchmark::DoNotOptimize(id);
        }

        benchmark::ClobberMemory();

        state.PauseTiming();

        if (!book->validate()) {
            state.SkipWithError(
                "Distributed insertion correctness check failed"
            );
            book.reset();
            break;
        }

        book.reset();

        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        state.iterations() * orderCount
    );
}

// ------------------------------------------------------------
// 2. Distributed bulk matching
// ------------------------------------------------------------

static void BM_DistributedMatching(
    benchmark::State& state,
    OrderSide incomingSide
) {
    const int orderCount = static_cast<int>(state.range(0));
    const int priceLevelCount = static_cast<int>(state.range(1));

    if (!validDistributionArguments(
            state, orderCount, priceLevelCount)) {
        return;
    }

    const OrderSide restingSide =
        incomingSide == OrderSide::BUY
            ? OrderSide::SELL
            : OrderSide::BUY;

    for (auto _ : state) {
        state.PauseTiming();

        auto book = std::make_unique<OrderBook>();

        // Prepare 10,000 resting orders across the requested
        // number of price levels.
        for (int i = 0; i < orderCount; ++i) {
            int id = book->addOrder(
                restingSide,
                distributedPrice(
                    restingSide,
                    i,
                    priceLevelCount
                ),
                1
            );

            benchmark::DoNotOptimize(id);
        }

        state.ResumeTiming();

        // Cross every resting price level and consume every order.
        const int64_t incomingPrice =
            incomingSide == OrderSide::BUY
                ? 100000 + priceLevelCount - 1
                : 100000 - priceLevelCount + 1;

        int incomingId = book->addOrder(
            incomingSide,
            incomingPrice,
            orderCount
        );

        benchmark::DoNotOptimize(incomingId);
        benchmark::DoNotOptimize(book->getTrades().size());
        benchmark::ClobberMemory();

        state.PauseTiming();

        if (incomingId <= 0 ||
            book->getTrades().size() !=
                static_cast<std::size_t>(orderCount) ||
            !book->validate()) {
            state.SkipWithError(
                "Distributed matching correctness check failed"
            );

            book.reset();
            break;
        }

        book.reset();

        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        state.iterations() * orderCount
    );
}

// ------------------------------------------------------------
// 3. Distributed cancellation
// ------------------------------------------------------------

static void BM_DistributedCancellation(
    benchmark::State& state,
    OrderSide side
) {
    const int orderCount = static_cast<int>(state.range(0));
    const int priceLevelCount = static_cast<int>(state.range(1));

    if (!validDistributionArguments(
            state, orderCount, priceLevelCount)) {
        return;
    }

    std::vector<int> orderIds;
    orderIds.reserve(orderCount);

    for (auto _ : state) {
        state.PauseTiming();

        auto book = std::make_unique<OrderBook>();
        orderIds.clear();

        for (int i = 0; i < orderCount; ++i) {
            const int id = book->addOrder(
                side,
                distributedPrice(side, i, priceLevelCount),
                10
            );

            orderIds.push_back(id);
        }

        state.ResumeTiming();

        bool allSucceeded = true;

        for (int id : orderIds) {
            bool cancelled = book->cancelOrder(id);

            benchmark::DoNotOptimize(cancelled);

            allSucceeded = cancelled && allSucceeded;
        }

        benchmark::ClobberMemory();

        state.PauseTiming();

        if (!allSucceeded || !book->validate()) {
            state.SkipWithError(
                "Distributed cancellation correctness check failed"
            );

            book.reset();
            break;
        }

        book.reset();

        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        state.iterations() * orderCount
    );
}

// ------------------------------------------------------------
// 4. Distributed quantity reduction
// ------------------------------------------------------------

static void BM_DistributedQuantityReduction(
    benchmark::State& state,
    OrderSide side
) {
    const int orderCount = static_cast<int>(state.range(0));
    const int priceLevelCount = static_cast<int>(state.range(1));

    if (!validDistributionArguments(
            state, orderCount, priceLevelCount)) {
        return;
    }

    std::vector<int> orderIds;
    orderIds.reserve(orderCount);

    const int ordersPerLevel =
        orderCount / priceLevelCount;

    for (auto _ : state) {
        state.PauseTiming();

        auto book = std::make_unique<OrderBook>();
        orderIds.clear();

        for (int i = 0; i < orderCount; ++i) {
            int id = book->addOrder(
                side,
                distributedPrice(side, i, priceLevelCount),
                10
            );

            orderIds.push_back(id);
        }

        state.ResumeTiming();

        bool allSucceeded = true;

        for (int id : orderIds) {
            int result = book->modifyOrder(
                id,
                distributedPrice(
                    side,
                    id - 1,
                    priceLevelCount
                ),
                5
            );

            benchmark::DoNotOptimize(result);

            allSucceeded =
                (result == id) && allSucceeded;
        }

        benchmark::ClobberMemory();

        state.PauseTiming();

        bool quantitiesCorrect = true;

        for (int level = 0;
             level < priceLevelCount;
             ++level) {

            const int64_t price =
                distributedPrice(side, level, priceLevelCount);

            const int expectedQuantity =
                ordersPerLevel * 5;

            if (book->getQuantityAtPrice(side, price) !=
                expectedQuantity) {
                quantitiesCorrect = false;
                break;
            }
        }

        if (!allSucceeded ||
            !quantitiesCorrect ||
            !book->validate()) {
            state.SkipWithError(
                "Distributed quantity reduction correctness check failed"
            );

            book.reset();
            break;
        }

        book.reset();

        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        state.iterations() * orderCount
    );
}

// ------------------------------------------------------------
// 5. Distributed price replacement
// ------------------------------------------------------------

static void BM_DistributedPriceReplacement(
    benchmark::State& state,
    OrderSide side
) {
    const int orderCount = static_cast<int>(state.range(0));
    const int priceLevelCount = static_cast<int>(state.range(1));

    if (!validDistributionArguments(
            state, orderCount, priceLevelCount)) {
        return;
    }

    std::vector<int> orderIds;
    orderIds.reserve(orderCount);

    for (auto _ : state) {
        state.PauseTiming();

        auto book = std::make_unique<OrderBook>();
        orderIds.clear();

        for (int i = 0; i < orderCount; ++i) {
            const int id = book->addOrder(
                side,
                distributedPrice(side, i, priceLevelCount),
                10
            );

            orderIds.push_back(id);
        }

        state.ResumeTiming();

        bool allSucceeded = true;

        for (int i = 0; i < orderCount; ++i) {
            int result = book->modifyOrder(
                orderIds[i],
                replacementPrice(
                    side,
                    i,
                    priceLevelCount
                ),
                10
            );

            const int expectedNewId =
                orderCount + i + 1;

            benchmark::DoNotOptimize(result);

            allSucceeded =
                (result == expectedNewId) && allSucceeded;
        }

        benchmark::ClobberMemory();

        state.PauseTiming();

        if (!allSucceeded || !book->validate()) {
            state.SkipWithError(
                "Distributed price replacement correctness check failed"
            );

            book.reset();
            break;
        }

        book.reset();

        state.ResumeTiming();
    }

    state.SetItemsProcessed(
        state.iterations() * orderCount
    );
}

// ------------------------------------------------------------
// Register BUY and SELL workloads
// ------------------------------------------------------------

#define REGISTER_DISTRIBUTION_BENCHMARK(function, sideName, sideValue) \
    BENCHMARK_CAPTURE(function, sideName, sideValue) \
        ->Args({10000, 100}) \
        ->Args({10000, 1000}) \
        ->Args({10000, 10000})

REGISTER_DISTRIBUTION_BENCHMARK(
    BM_DistributedAddOrders, BUY, OrderSide::BUY);

REGISTER_DISTRIBUTION_BENCHMARK(
    BM_DistributedAddOrders, SELL, OrderSide::SELL);

REGISTER_DISTRIBUTION_BENCHMARK(
    BM_DistributedMatching, BUY, OrderSide::BUY);

REGISTER_DISTRIBUTION_BENCHMARK(
    BM_DistributedMatching, SELL, OrderSide::SELL);

REGISTER_DISTRIBUTION_BENCHMARK(
    BM_DistributedCancellation, BUY, OrderSide::BUY);

REGISTER_DISTRIBUTION_BENCHMARK(
    BM_DistributedCancellation, SELL, OrderSide::SELL);

REGISTER_DISTRIBUTION_BENCHMARK(
    BM_DistributedQuantityReduction, BUY, OrderSide::BUY);

REGISTER_DISTRIBUTION_BENCHMARK(
    BM_DistributedQuantityReduction, SELL, OrderSide::SELL);

REGISTER_DISTRIBUTION_BENCHMARK(
    BM_DistributedPriceReplacement, BUY, OrderSide::BUY);

REGISTER_DISTRIBUTION_BENCHMARK(
    BM_DistributedPriceReplacement, SELL, OrderSide::SELL);

#undef REGISTER_DISTRIBUTION_BENCHMARK

BENCHMARK(BM_AddBuyOrders)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

BENCHMARK(BM_AddSellOrders)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

BENCHMARK(BM_MatchBuyAgainstAsks)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

BENCHMARK(BM_MatchSellAgainstBids)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

BENCHMARK(BM_CancelBuyOrders)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

BENCHMARK(BM_CancelSellOrders)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);


BENCHMARK(BM_ReduceBuyOrderQuantities)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

BENCHMARK(BM_ReduceSellOrderQuantities)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

BENCHMARK(BM_ReplaceBuyOrders)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

BENCHMARK(BM_ReplaceSellOrders)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);
