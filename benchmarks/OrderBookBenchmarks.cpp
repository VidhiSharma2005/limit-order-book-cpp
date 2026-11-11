
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
