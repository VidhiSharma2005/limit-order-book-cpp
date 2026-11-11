
cmake_minimum_required(VERSION 3.16)

project(LimitOrderBook VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Core matching engine library
add_library(OrderBookCore
    src/OrderBook.cpp
)

target_include_directories(OrderBookCore PUBLIC include)

# Exchange simulator
add_executable(LimitOrderBook
    src/main.cpp
)

target_link_libraries(LimitOrderBook PRIVATE OrderBookCore)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(OrderBookCore PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow
    )

    target_compile_options(LimitOrderBook PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow
    )
endif()

# Automated testing
enable_testing()

find_package(GTest CONFIG REQUIRED)

add_executable(OrderBookTests
    tests/OrderBookTests.cpp
    src/ReplayEngine.cpp
)

target_link_libraries(OrderBookTests PRIVATE
    OrderBookCore
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(OrderBookTests)

# Performance benchmarks (optional)
option(BUILD_BENCHMARKS "Build Google Benchmark targets" OFF)

if(BUILD_BENCHMARKS)
    find_package(benchmark CONFIG REQUIRED)

    add_executable(order_book_benchmarks
        benchmarks/OrderBookBenchmarks.cpp
    )

    target_link_libraries(order_book_benchmarks PRIVATE
        OrderBookCore
        benchmark::benchmark_main
    )

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(order_book_benchmarks PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow
        )
    endif()
endif()

# Individual operation latency measurements
add_executable(order_book_latency
    benchmarks/OrderBookLatency.cpp
)

target_link_libraries(order_book_latency PRIVATE
    OrderBookCore
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(order_book_latency PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow
    )
endif()