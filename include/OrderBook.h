#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include "Order.h"
#include "Trade.h"

#include <map>
#include <unordered_map>
#include <list>
#include <vector>
#include <functional>

class OrderBook {

private:
    // Buy orders: highest price first
    std::map<int64_t, std::list<Order>, std::greater<int64_t>> bids;

    // Sell orders: lowest price first
    std::map<int64_t, std::list<Order>> asks;

    struct OrderLocation {
        OrderSide side;
        int64_t price;
        std::list<Order>::iterator iterator;
    };

    // Fast lookup of orders by their IDs
    std::unordered_map<int, OrderLocation> orderLookup;

    std::vector<Trade> trades;
    int nextOrderId = 1;

    void matchOrder(Order& incomingOrder);


public:
    const std::vector<Trade>& getTrades() const;

    int getQuantityAtPrice(OrderSide side, int64_t price) const;

    // Adds a new order to the order book
    int addOrder(OrderSide side, int64_t price, int quantity);

    // Displays the current order book
    void displayOrderBook() const;

    // Displays executed trades
    void displayTrades() const;

    bool hasOrder(int orderId) const;

    bool cancelOrder(int orderId);

    int modifyOrder(int orderId, int64_t newPrice, int newQuantity);

    bool validate() const;

    int peekNextOrderId() const {
    return nextOrderId;
}

int previewModifyOrderId(
    int orderId,
    int64_t newPrice,
    int newQuantity
) const;
};

#endif