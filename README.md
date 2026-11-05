# Limit Order Book & Matching Engine

A C++ implementation of a limit order book and matching engine, exploring core concepts used in electronic trading systems and financial market infrastructure.

The project focuses on price-time priority, order matching, partial executions, order lifecycle management, and efficient data structures.

## Features

- Limit buy and sell orders
- Bid and ask order books
- Price-time priority
- Order matching and trade execution
- Partial order fills
- Order cancellation
- Order modification
- Order validation
- Automated unit testing

## Tech Stack

- **Language:** C++
- **Build System:** CMake
- **Build Tool:** Ninja
- **Core Libraries:** C++ Standard Template Library (STL)
- **Testing:** CTest

## Project Structure

```text
LimitOrderBook/
├── CMakeLists.txt
├── include/
│   ├── Order.h
│   ├── OrderBook.h
│   └── Trade.h
├── src/
│   ├── main.cpp
│   └── OrderBook.cpp
├── tests/
│   └── OrderBookTests.cpp
└── README.md
```

## Build and Run

### Prerequisites

- C++ compiler with C++17 support
- CMake
- Ninja build system
- Git

Configure the project:

```bash
cmake -S . -B build -G Ninja
```

Build:

```bash
cmake --build build
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

The project currently includes 26 test cases covering order validation,
price-time priority, matching, partial fills, cancellation, and modification.

## Learning Objectives

- Understand limit order books and electronic trading systems.
- Implement price-time priority matching.
- Explore efficient STL data structures.
- Maintain order book consistency through order lifecycle operations.
- Develop reliable unit tests for matching engine behavior.
- Evaluate performance and scalability.

## Roadmap

- [x] Basic limit order book
- [x] Price-time priority
- [x] Partial fills and trade execution
- [x] Order cancellation
- [x] Order modification
- [x] Unit testing
- [ ] Performance benchmarking using Google Benchmark
- [ ] Optimize order book data structures
- [ ] Measure throughput and latency
- [ ] Explore concurrency and scalability

## Motivation

This project is an exploration of the intersection of computer science and financial markets, with an emphasis on understanding the engineering principles behind trading infrastructure.

More improvements and experiments will be documented as development progresses.