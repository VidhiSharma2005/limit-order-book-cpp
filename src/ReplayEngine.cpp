#include "ReplayEngine.h"

ReplayResult ReplayEngine::replay(
    const std::vector<OrderEvent>& events,
    OrderBook& book
) const {

    std::size_t processedEvents = 0;
    std::uint64_t expectedSequenceNumber = 1;

    for (const OrderEvent& event : events) {
        if (event.sequenceNumber != expectedSequenceNumber) {
    return {
        false,
        processedEvents,
        "Event " + std::to_string(event.sequenceNumber) +
        ": unexpected sequence number (expected " +
        std::to_string(expectedSequenceNumber) + ")"
    };
       }
        // Process the event according to its type.
        switch (event.type) {

            case EventType::NEW_ORDER: {
                if ((event.side != OrderSide::BUY &&
     event.side != OrderSide::SELL) ||
    event.price <= 0 ||
    event.quantity <= 0) {
    return {
        false,
        processedEvents,
        "Event " + std::to_string(event.sequenceNumber) +
        ": new order failed"
    };
}
                if (event.orderId != book.peekNextOrderId()) {
    return {
        false,
        processedEvents,
        "Event " + std::to_string(event.sequenceNumber) +
        ": generated order ID mismatch"
    };
}

                
                int generatedId = book.addOrder(
                    event.side,
                    event.price,
                    event.quantity
                );

                if (generatedId == -1) {
                    return {
                        false,
                        processedEvents,
                        "Event " + std::to_string(event.sequenceNumber) +
                        ": new order failed"
                    };
                }

                if (event.orderId != generatedId) {
                    return {
                        false,
                        processedEvents,
                        "Event " + std::to_string(event.sequenceNumber) +
                        ": generated order ID mismatch"
                    };
                }

                break;
            }

            case EventType::CANCEL_ORDER: {

                bool cancelled = book.cancelOrder(event.orderId);

                if (!cancelled) {
                    return {
                        false,
                        processedEvents,
                        "Event " + std::to_string(event.sequenceNumber) +
                        ": cancellation failed"
                    };
                }

                break;
            }

            case EventType::MODIFY_ORDER: {
                if (event.price <= 0 || event.quantity <= 0) {
    return {
        false,
        processedEvents,
        "Event " + std::to_string(event.sequenceNumber) +
        ": modification failed"
    };
}
                if (event.expectedResultOrderId != -1) {
    int predictedId = book.previewModifyOrderId(
        event.orderId,
        event.price,
        event.quantity
    );

    if (predictedId != event.expectedResultOrderId) {
        return {
            false,
            processedEvents,
            "Event " + std::to_string(event.sequenceNumber) +
            ": modified order ID mismatch"
        };
    }
}
                int modifiedId = book.modifyOrder(
                    event.orderId,
                    event.price,
                    event.quantity
                );

                if (modifiedId == -1) {
                    return {
                        false,
                        processedEvents,
                        "Event " + std::to_string(event.sequenceNumber) +
                        ": modification failed"
                    };
                }

                if (event.expectedResultOrderId != -1 &&
                    modifiedId != event.expectedResultOrderId) {
                    return {
                        false,
                        processedEvents,
                        "Event " + std::to_string(event.sequenceNumber) +
                        ": modified order ID mismatch"
                    };
                }

                break;
            }

            default:
                return {
                    false,
                    processedEvents,
                    "Event " + std::to_string(event.sequenceNumber) +
                    ": unknown event type"
                };
        }

        // Check internal order-book consistency after each event.
        if (!book.validate()) {
            return {
                false,
                processedEvents,
                "Event " + std::to_string(event.sequenceNumber) +
                ": order book invariant violation"
            };
        }

        processedEvents++;
        expectedSequenceNumber++;
    }

    return {
        true,
        processedEvents,
        ""
    };
}
