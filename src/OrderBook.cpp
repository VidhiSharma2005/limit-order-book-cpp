
#include "OrderBook.h"

#include <algorithm>
#include <iostream>
#include <iterator>

// Adds a new order and attempts to match it immediately.
int OrderBook::addOrder(OrderSide side, int64_t price, int quantity) {

    // Validate the incoming order.
    if (price <= 0 || quantity <= 0) {
        std::cout << "Invalid order: price and quantity must be positive.\n";
        return -1;
    }

    // Assign a unique ID to the incoming order.
    Order incomingOrder(nextOrderId++, side, price, quantity);

    std::cout << "\nNew order received: ID=" << incomingOrder.orderId
              << ", Side=" << (side == OrderSide::BUY ? "BUY" : "SELL")
              << ", Price=" << price
              << ", Quantity=" << quantity << '\n';

    // Try to match it against the opposite side of the book.
    matchOrder(incomingOrder);

    // Add any unfilled quantity to the appropriate side.
    
    
    if (incomingOrder.quantity > 0) {
        auto& ordersAtPrice =
            (side == OrderSide::BUY) ? bids[price] : asks[price];

        ordersAtPrice.push_back(incomingOrder);

        auto orderIterator = std::prev(ordersAtPrice.end());

        orderLookup.emplace(
            incomingOrder.orderId,
            OrderLocation{
                side,
                price,
                orderIterator
            }
        );

        std::cout << "Remaining quantity " << incomingOrder.quantity
                << " added to the order book.\n";
    } else {
        std::cout << "Order completely filled.\n";
    }


    return incomingOrder.orderId;
}


// Matches an incoming order against resting orders.

void OrderBook::matchOrder(Order& incomingOrder) {

    // Incoming BUY matches against existing SELL orders.
    if (incomingOrder.side == OrderSide::BUY) {

        while (incomingOrder.quantity > 0 && !asks.empty()) {

            // Lowest available selling price.
            auto bestAsk = asks.begin();

            // Stop if the incoming BUY price cannot cross the best ASK.
            if (incomingOrder.price < bestAsk->first) {
                break;
            }

            // Oldest order at the best ASK price.
            Order& restingOrder = bestAsk->second.front();

            int tradedQuantity = std::min(
                incomingOrder.quantity,
                restingOrder.quantity
            );

            // Execute at the resting order's price.
            trades.push_back({
                incomingOrder.orderId,
                restingOrder.orderId,
                restingOrder.price,
                tradedQuantity
            });

            std::cout << "TRADE: Buy Order " << incomingOrder.orderId
                      << " matched with Sell Order " << restingOrder.orderId
                      << " at price " << restingOrder.price
                      << ", Quantity=" << tradedQuantity << '\n';

            // Update quantities.
            incomingOrder.quantity -= tradedQuantity;
            restingOrder.quantity -= tradedQuantity;

            // Remove fully executed order from lookup and price level.
            if (restingOrder.quantity == 0) {
                orderLookup.erase(restingOrder.orderId);
                bestAsk->second.pop_front();
            }

            // Remove empty ASK price level.
            if (bestAsk->second.empty()) {
                asks.erase(bestAsk);
            }
        }
    }

    // Incoming SELL matches against existing BUY orders.
    else {

        while (incomingOrder.quantity > 0 && !bids.empty()) {

            // Highest available buying price.
            auto bestBid = bids.begin();

            // Stop if the incoming SELL price cannot cross the best BID.
            if (incomingOrder.price > bestBid->first) {
                break;
            }

            // Oldest order at the best BID price.
            Order& restingOrder = bestBid->second.front();

            int tradedQuantity = std::min(
                incomingOrder.quantity,
                restingOrder.quantity
            );

            // Execute at the resting order's price.
            trades.push_back({
                restingOrder.orderId,
                incomingOrder.orderId,
                restingOrder.price,
                tradedQuantity
            });

            std::cout << "TRADE: Sell Order " << incomingOrder.orderId
                      << " matched with Buy Order " << restingOrder.orderId
                      << " at price " << restingOrder.price
                      << ", Quantity=" << tradedQuantity << '\n';

            // Update quantities.
            incomingOrder.quantity -= tradedQuantity;
            restingOrder.quantity -= tradedQuantity;

            // Remove fully executed order from lookup and price level.
            if (restingOrder.quantity == 0) {
                orderLookup.erase(restingOrder.orderId);
                bestBid->second.pop_front();
            }

            // Remove empty BID price level.
            if (bestBid->second.empty()) {
                bids.erase(bestBid);
            }
        }
    }
}



// Displays the current state of the order book.
void OrderBook::displayOrderBook() const {

    std::cout << "\n========== ORDER BOOK ==========\n";

    std::cout << "\nASKS (SELL ORDERS)\n";

    for (const auto& [price, orders] : asks) {

        int totalQuantity = 0;

        for (const auto& order : orders) {
            totalQuantity += order.quantity;
        }

        std::cout << "Price: " << price
                  << " | Quantity: " << totalQuantity << '\n';
    }

    std::cout << "\nBIDS (BUY ORDERS)\n";

    for (const auto& [price, orders] : bids) {

        int totalQuantity = 0;

        for (const auto& order : orders) {
            totalQuantity += order.quantity;
        }

        std::cout << "Price: " << price
                  << " | Quantity: " << totalQuantity << '\n';
    }

    std::cout << "\n================================\n";
}


// Displays all executed trades.
void OrderBook::displayTrades() const {

    std::cout << "\n========== TRADE HISTORY ==========\n";

    if (trades.empty()) {
        std::cout << "No trades executed yet.\n";
        return;
    }

    for (const auto& trade : trades) {

        std::cout << "Buy Order: " << trade.buyOrderId
                  << " | Sell Order: " << trade.sellOrderId
                  << " | Price: " << trade.executionPrice
                  << " | Quantity: " << trade.quantity << '\n';
    }

    std::cout << "===================================\n";
}

// Returns trade history without allowing external modification.
const std::vector<Trade>& OrderBook::getTrades() const {
    return trades;
}


// Returns the total remaining quantity at a particular price level.
int OrderBook::getQuantityAtPrice(OrderSide side, int64_t price) const {

    if (side == OrderSide::BUY) {

        auto it = bids.find(price);

        if (it == bids.end()) {
            return 0;
        }

        int totalQuantity = 0;

        for (const auto& order : it->second) {
            totalQuantity += order.quantity;
        }

        return totalQuantity;
    }

    else {

        auto it = asks.find(price);

        if (it == asks.end()) {
            return 0;
        }

        int totalQuantity = 0;

        for (const auto& order : it->second) {
            totalQuantity += order.quantity;
        }

        return totalQuantity;
    }
}

bool OrderBook::hasOrder(int orderId) const {
    return orderLookup.find(orderId) != orderLookup.end();
}


bool OrderBook::cancelOrder(int orderId) {

    // Find the order in the lookup map.
    auto lookupIt = orderLookup.find(orderId);

    // Order doesn't exist or was already executed/cancelled.
    if (lookupIt == orderLookup.end()) {
        return false;
    }

    // Copy location before removing lookup entry.
    OrderLocation location = lookupIt->second;

    if (location.side == OrderSide::BUY) {

        auto priceLevel = bids.find(location.price);

        if (priceLevel == bids.end()) {
            return false;
        }

        // Remove the specific BUY order.
        priceLevel->second.erase(location.iterator);

        // Remove empty price level.
        if (priceLevel->second.empty()) {
            bids.erase(priceLevel);
        }
    }

    else {

        auto priceLevel = asks.find(location.price);

        if (priceLevel == asks.end()) {
            return false;
        }

        // Remove the specific SELL order.
        priceLevel->second.erase(location.iterator);

        // Remove empty price level.
        if (priceLevel->second.empty()) {
            asks.erase(priceLevel);
        }
    }

    // Remove the lookup entry after successful book removal.
    orderLookup.erase(lookupIt);

    std::cout << "Order " << orderId << " cancelled successfully.\n";

    return true;
}



int OrderBook::modifyOrder(
    int orderId,
    int64_t newPrice,
    int newQuantity
) {
    // Validate modification parameters.
    if (newPrice <= 0 || newQuantity <= 0) {
        return -1;
    }

    // Find the existing order.
    auto lookupIt = orderLookup.find(orderId);

    if (lookupIt == orderLookup.end()) {
        return -1;
    }

    // Get the existing order.
    OrderLocation location = lookupIt->second;
    Order& existingOrder = *location.iterator;

    // Case 1: Price is unchanged and quantity is unchanged.
    if (newPrice == existingOrder.price &&
        newQuantity == existingOrder.quantity) {
        return orderId;
    }

    // Case 2: Same price, reduced quantity.
    // Update in place to preserve time priority.
    if (newPrice == existingOrder.price &&
        newQuantity < existingOrder.quantity) {

        existingOrder.quantity = newQuantity;

        std::cout << "Order " << orderId
                  << " quantity reduced to " << newQuantity << '\n';

        return orderId;
    }

    // Case 3: Price changed or quantity increased.
    // Cancel the old order and submit a replacement.
    OrderSide side = location.side;

    if (!cancelOrder(orderId)) {
        return -1;
    }

    int newOrderId = addOrder(side, newPrice, newQuantity);

    return newOrderId;
}

bool OrderBook::validate() const {

    std::size_t restingOrderCount = 0;

    // Validate all buy orders.
    for (const auto& [price, orders] : bids) {

        // Every price level must contain at least one order.
        if (orders.empty()) {
            return false;
        }

        for (auto it = orders.begin(); it != orders.end(); ++it) {

            const Order& order = *it;
            restingOrderCount++;

            // Order must have the correct side.
            if (order.side != OrderSide::BUY) {
                return false;
            }

            // Order price must match its map key.
            if (order.price != price) {
                return false;
            }

            // Resting orders must have positive quantity.
            if (order.quantity <= 0) {
                return false;
            }

            // Order must exist in the lookup map.
            auto lookupIt = orderLookup.find(order.orderId);

            if (lookupIt == orderLookup.end()) {
                return false;
            }

            const OrderLocation& location = lookupIt->second;

            // Lookup must point to the correct order.
            if (location.side != OrderSide::BUY ||
                location.price != price ||
                location.iterator != it) {
                return false;
            }
        }
    }

    // Validate all sell orders.
    for (const auto& [price, orders] : asks) {

        if (orders.empty()) {
            return false;
        }

        for (auto it = orders.begin(); it != orders.end(); ++it) {

            const Order& order = *it;
            restingOrderCount++;

            if (order.side != OrderSide::SELL) {
                return false;
            }

            if (order.price != price) {
                return false;
            }

            if (order.quantity <= 0) {
                return false;
            }

            auto lookupIt = orderLookup.find(order.orderId);

            if (lookupIt == orderLookup.end()) {
                return false;
            }

            const OrderLocation& location = lookupIt->second;

            if (location.side != OrderSide::SELL ||
                location.price != price ||
                location.iterator != it) {
                return false;
            }
        }
    }

    // Every resting order must have exactly one lookup entry.
    if (restingOrderCount != orderLookup.size()) {
        return false;
    }

    return true;
}