#include "ob/QueueTracker.h"

#include <cstdint>
#include "ob/OrderBook.h"
#include <cassert>


bool QueueTracker::startTracking(std::int64_t id, const OrderBook& orderBook) {

    const auto orderInfo {orderBook.getOrderInfo(id)};
    if (!orderInfo) return false;
    if (m_trackedOrders.contains(id)) return false;
    TrackedOrder trackedOrder{orderInfo->side, orderInfo->price, *orderBook.queueAhead(id), orderInfo->ticket};
    return m_trackedOrders.insert({id, trackedOrder}).second;
}


std::optional<std::int64_t> QueueTracker::ahead(std::int64_t id) const {
    // Finds ahead quantity orders.
    const auto ind {m_trackedOrders.find(id)};
    if (ind == m_trackedOrders.end()) return std::nullopt;
    return ind->second.ahead;
}

void QueueTracker::updateBeforeApply(const OrderEvent &event, const OrderBook &orderBook) {
    if (event.type == OrderType::Add) return; // if the order is new it doesn't impact
    const auto orderNew{orderBook.getOrderInfo(event.orderId)};
    // if order isn't new it has an orderId and thus info (checking if it exists later)
    if (!orderNew) return;
    std::int64_t newQty{event.quantity};
    std::int64_t modifiedQty{newQty - orderNew->quantity};
    for (auto &[key, value]: m_trackedOrders) {
        // We will check among all the orders we are tracking, if the new one is in the same level
        if (key == event.orderId) continue;
        if (orderNew->price == value.price && orderNew->ticket < value.ticket && orderNew->side == value.side) {
            switch (event.type) {
                case OrderType::Execute:
                    value.ahead -= newQty;
                    break;
                case OrderType::Cancel:
                    value.ahead -= orderNew->quantity;
                    break;
                case OrderType::Modify:
                    if (modifiedQty < 0) {
                        value.ahead += modifiedQty;
                    }
                    if (modifiedQty > 0) {
                        value.ahead -= orderNew->quantity;
                    }
                    break;
                default:
                    assert(false && "updateBeforeApply Queue tracker");
                    return;
            }
        }
    }
}


void QueueTracker::updateAfterApply(const OrderEvent &event, const OrderBook &orderBook) {
    auto it{m_trackedOrders.find(event.orderId)};


    if (it != m_trackedOrders.end()) {
        const auto orderInfo{orderBook.getOrderInfo(event.orderId)};
        if (!orderInfo) {
            this->m_trackedOrders.erase(it);
            return;
        }
        if (orderInfo->ticket != it->second.ticket) {
            it->second.ticket = orderInfo->ticket;
            it->second.ahead = *orderBook.queueAhead(event.orderId);
        }
    } else {
        if (event.type == OrderType::Add) {
            for (auto trackedIt{m_trackedOrders.begin()}; trackedIt != m_trackedOrders.end();) {
                if (!orderBook.getOrderInfo(trackedIt->first)) // if this tracked order is gone from the book
                {
                    trackedIt = m_trackedOrders.erase(trackedIt); // erase returns the next one
                } else {
                    ++trackedIt;
                }
            }
        }
        return;
    }
}
