#ifndef ORDER_H
#define ORDER_H

#include <cstdint>

enum class OrderSide {
    BUY,
    SELL
};

struct Order {
    int orderId;
    OrderSide side;
    int64_t price;
    int quantity;

    Order(int id, OrderSide orderSide, int64_t orderPrice, int orderQuantity)
        : orderId(id),
          side(orderSide),
          price(orderPrice),
          quantity(orderQuantity) {}
};

#endif