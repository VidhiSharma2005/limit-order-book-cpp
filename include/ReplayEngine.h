#ifndef REPLAY_ENGINE_H
#define REPLAY_ENGINE_H

#include "Event.h"
#include "OrderBook.h"

#include <vector>
#include <string>

struct ReplayResult {
    bool success;
    std::size_t processedEvents;
    std::string errorMessage;
};

class ReplayEngine {
public:
    ReplayResult replay(
        const std::vector<OrderEvent>& events,
        OrderBook& book
    ) const;
};

#endif
