//
// Created by mouad-sehli on 11/8/26.
//

#include "ob/OrderBook.h"
#include <cassert>
#include <iterator>

BookStatus OrderBook::add(const OrderEvent& event) {

    // if not add -> wrong function, if already in index -> duplicate, if <= qty -> Invalid
    if (event.type!= OrderType::Add) return BookStatus::InvalidOrder;
    if (m_index.contains(event.orderId)) return BookStatus::DuplicateId;
    if (event.quantity <= 0) return BookStatus::InvalidOrder;
    if (event.price <= 0) return BookStatus::InvalidOrder;

    switch (event.side) {
        case Side::Buy: {
            // Find the level or create it and fill order
            auto [levelIt, created] = m_bidSide.try_emplace(event.price);
            Order order = {event.orderId, event.quantity};

            Level &level{levelIt->second}; // find the level stored in bid map then do the required changes
            level.queue.push_back(order);
            level.totalQty += event.quantity;
            level.price = event.price;

            // get the iterator of the last element of the queue and store it in location
            auto prevIt = std::prev(level.queue.end());
            Location location = {event.side, event.price, prevIt};
            // fill the index
            m_index.insert({event.orderId, location});
            return BookStatus::Ok;
        }
        case Side::Sell: {
            auto [levelIt, created] = m_askSide.try_emplace(event.price);
            Order order = {event.orderId, event.quantity};

            Level &level{levelIt->second};
            level.queue.push_back(order);
            level.totalQty += event.quantity;
            level.price = event.price;

            auto prevIt = std::prev(level.queue.end());
            Location location = {event.side, event.price, prevIt};

            m_index.insert({event.orderId, location});

            return BookStatus::Ok;
        }
        case Side::None:
            return BookStatus::InvalidOrder;
        default:
            assert(false && " Invalid order type");
            return BookStatus::InvalidOrder; // because asserts are removed when built
        }

}

std::optional<TopOfBook> OrderBook::bestAsk() const {
    if (m_askSide.empty()) return std::nullopt;
    // if not empty look for the total quantity of the level associated with the best price
    TopOfBook topOfBook = {m_askSide.begin()->first, m_askSide.begin()->second.totalQty};
    return topOfBook;
}

std::optional<TopOfBook> OrderBook::bestBid() const {
    if (m_bidSide.empty()) return std::nullopt;
    TopOfBook topOfBook = {m_bidSide.begin()->first, m_bidSide.begin()->second.totalQty};
    return topOfBook;

}