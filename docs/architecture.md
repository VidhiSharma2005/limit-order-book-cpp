# Architecture – Limit Order Book & Matching Engine

## 1. Overview

The Limit Order Book is implemented in C++ using ordered maps, linked lists, and an unordered map to manage buy and sell orders efficiently.

The matching engine follows price-time priority:

1. Better-priced orders receive execution priority.
2. Orders at the same price are executed in arrival order.

Incoming orders are matched against the opposite side of the book. Any remaining quantity is added to the appropriate side as a resting order.

## 2. High-Level Architecture

The primary order-processing flow is:

1. Receive an incoming order.
2. Validate its price and quantity.
3. Assign a unique order ID.
4. Attempt to match against the opposite side.
5. Record executed trades.
6. Add any remaining quantity to the order book.
7. Update the active-order lookup.

The main components are:

- `addOrder()` – validates, assigns IDs, initiates matching and stores remaining quantities.
- `matchOrder()` – executes matching according to price-time priority.
- `cancelOrder()` – removes an active resting order.
- `modifyOrder()` – modifies an existing resting order.
- `validate()` – checks consistency between the order book and lookup structure.

## 3. Data Structures

### 3.1 Bid Side

```cpp
std::map<int64_t, std::list<Order>,
         std::greater<int64_t>> bids;
```

The bid side stores buy orders.

- Prices are sorted in descending order.
- The highest buy price is accessible first.
- Each price level contains a linked list of orders.

### 3.2 Ask Side

```cpp
std::map<int64_t, std::list<Order>> asks;
```

The ask side stores sell orders.

- Prices are sorted in ascending order.
- The lowest sell price is accessible first.
- Each price level contains a linked list of orders.

### 3.3 Order Queues

Each price level maintains a `std::list<Order>`.

Orders are appended to the end of the list and matched from the front.

This preserves FIFO ordering for orders at the same price.

### 3.4 Order Lookup

```cpp
std::unordered_map<int, OrderLocation> orderLookup;
```

The lookup maps an order ID to its location in the order book.

`OrderLocation` stores:

- Order side
- Price level
- Iterator to the corresponding order in the linked list

This avoids scanning all price levels and orders when locating an order for cancellation or modification.

### 3.5 Trade History

```cpp
std::vector<Trade> trades;
```

Executed trades are stored in a vector and exposed through a const reference using `getTrades()`.

## 4. Matching Algorithm

The matching engine processes incoming orders against the opposite side of the book.

### 4.1 Incoming Buy Order

A buy order matches against existing sell orders.

Conditions:

- The lowest ask is selected first.
- Matching continues while the incoming buy price is greater than or equal to the best ask.
- The oldest order at that ask price is selected first.

Execution price is the resting sell order's price.

### 4.2 Incoming Sell Order

A sell order matches against existing buy orders.

Conditions:

- The highest bid is selected first.
- Matching continues while the incoming sell price is less than or equal to the best bid.
- The oldest order at that bid price is selected first.

Execution price is the resting buy order's price.

### 4.3 Partial Fills

The executed quantity is:

```cpp
int tradedQuantity = std::min(
    incomingOrder.quantity,
    restingOrder.quantity
);
```

Both order quantities are reduced by the executed amount.

If a resting order reaches zero quantity, it is removed from:

- Its price-level list
- The order lookup map

Empty price levels are also removed.

If the incoming order retains quantity after matching, the remaining quantity is inserted into the appropriate side of the book.

## 5. Order Lifecycle Management

### 5.1 Adding Orders

`addOrder()`:

1. Rejects non-positive prices and quantities.
2. Assigns a unique order ID.
3. Calls `matchOrder()`.
4. Stores any remaining quantity.
5. Adds the active order to `orderLookup`.

### 5.2 Cancelling Orders

`cancelOrder()`:

1. Locates the order using `orderLookup`.
2. Retrieves its side, price and list iterator.
3. Erases the order from its price-level list.
4. Removes an empty price level.
5. Erases the lookup entry.

The operation avoids scanning the entire order book to find the order.

### 5.3 Modifying Orders

`modifyOrder()` handles three important cases:

**No-op modification**

If neither price nor quantity changes, the existing order ID is returned without changing the order.

**Quantity reduction at the same price**

The quantity is updated in place. The order retains its position in the FIFO queue and therefore preserves time priority.

**Price change or quantity increase**

The original order is cancelled and a replacement order is submitted with a new ID.

The replacement loses its previous time priority and is processed as a new incoming order.

## 6. Order Book Consistency

The `validate()` function checks important invariants:

- No empty price levels exist.
- Buy orders are stored on the bid side.
- Sell orders are stored on the ask side.
- Order prices match their price-level keys.
- Resting orders have positive quantities.
- Every resting order has a corresponding lookup entry.
- Every lookup entry points to the correct order.
- The number of resting orders equals the number of lookup entries.

These checks help detect inconsistencies between the primary order book and its lookup structure.

## 7. Complexity Analysis

Let:

- P = number of price levels.
- N = number of resting orders.
- K = number of resting orders matched.
- L = number of price levels emptied during matching.

| Operation | Expected complexity |
|---|---|
| Access best bid/ask | O(1) |
| Insert a new price level | O(log P) |
| Append to a price-level queue | O(1) |
| Find order by ID | O(1) average |
| Cancel order | O(log P) overall |
| Reduce quantity in place | O(1) |
| No-op modification | O(1) average |
| Validate order book | O(N) average |
| Match incoming order | O(K + L log P), excluding any subsequent insertion of the remaining order |

The unordered map provides average constant-time lookup, while the ordered maps maintain price ordering.

These are analytical complexity estimates, not measured performance results.

## 8. Current Scope and Future Work

The current implementation focuses on the core mechanics of a single-book limit order matching engine.

Planned improvements include:

- Performance benchmarking using Google Benchmark.
- Throughput and latency measurements.
- Further data-structure optimization.
- Exploring concurrency and scalability.

