#ifndef EVENT_H
#define EVENT_H

#include "Order.h"

#include <cstdint>

enum class EventType {
    NEW_ORDER,
    CANCEL_ORDER,
    MODIFY_ORDER
};

struct OrderEvent {
    // Position of this event in the event sequence.
    std::uint64_t sequenceNumber;

    // Type of operation.
    EventType type;

    // For NEW_ORDER: expected generated order ID.
    // For CANCEL_ORDER / MODIFY_ORDER: existing order ID.
    int orderId;

    // Used by NEW_ORDER.
    OrderSide side;

    // Used by NEW_ORDER / MODIFY_ORDER.
    std::int64_t price;

    // Used by NEW_ORDER / MODIFY_ORDER.
    int quantity;

    // For MODIFY_ORDER: expected returned order ID.
    // -1 means not specified.
    int expectedResultOrderId = -1;
};

#endif
