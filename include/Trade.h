#ifndef TRADE_H
#define TRADE_H

#include <cstdint>

struct Trade {
    int buyOrderId;
    int sellOrderId;
    int64_t executionPrice;
    int quantity;
};

#endif