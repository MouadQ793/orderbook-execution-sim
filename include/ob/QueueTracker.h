#pragma once
#include "ob/OrderBook.h"
#include <unordered_map>
#include <cstdint>
#include <optional>

class QueueTracker {
public :
    bool startTracking(std::int64_t id, const OrderBook& orderBook);
    void updateBeforeApply(const OrderEvent& event, const OrderBook& orderBook);
    void updateAfterApply(const OrderEvent& event, const OrderBook& orderBook);
    std::optional<std::int64_t> ahead(std::int64_t id) const;
private:
    struct TrackedOrder {
        Side side{};
        std::int64_t price{};
        std::int64_t ahead{};
        std::int64_t ticket{};
    };
    std::unordered_map<std::int64_t, TrackedOrder> m_trackedOrders;
};