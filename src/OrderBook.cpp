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

BookStatus OrderBook::cancel(std::int64_t id) {
    const auto ind = m_index.find(id);
    if (ind == m_index.end()) return BookStatus::UnknownId;
    removeOrder(ind);
    return BookStatus::Ok;
}

void OrderBook::removeOrder(IndexIt ind) {
    Location location = {ind -> second};
    switch (location.side) {
        case Side::Buy: {
            // Find the id in the index to get location
            // for the given id find the associated level,
            // in the level use iterator from associated location to remove order from queue
            // remove quantity from total
            // then delete the level if qty is 0
            // finally remove the id from index
            Level& level {m_bidSide.find(location.price)->second};
            level.totalQty -= location.pt->quantity;
            level.queue.erase(location.pt);
            if (level.queue.empty()) m_bidSide.erase(location.price);
            m_index.erase(ind);
            return;
        }
        case Side::Sell: {
            Level& level {m_askSide.find(location.price)->second};
            level.totalQty -= location.pt->quantity;
            level.queue.erase(location.pt);
            if (level.queue.empty()) m_askSide.erase(location.price);
            m_index.erase(ind);
            return ;
        }
        case Side::None:
        default:
            assert(false && " an order in the index has no side");
            return ;
    }
}


BookStatus OrderBook::execute(std::int64_t id, std::int64_t quantity) {
    const auto ind = m_index.find(id);
    if (ind == m_index.end()) return BookStatus::UnknownId;
    // negative quantity is a mistake
    if (quantity <= 0 ) return BookStatus::InvalidOrder;

    Location& location {ind->second};
    std::int64_t remainingQty {ind->second.pt->quantity};
    // we compare the quantities to deem if the order is equivalent to cancel, undoable, or doable.
    if (quantity > remainingQty) return BookStatus::ExcessQuantity;
    if (quantity == remainingQty) {
        removeOrder(ind);
        return BookStatus::Ok;
    }
    switch (location.side) {
        case Side::Buy: {
            Level &level{m_bidSide.find(location.price)->second};
            level.totalQty -= quantity;
            location.pt->quantity -= quantity;
            break;
        }
        case Side::Sell: {
            Level &level{m_askSide.find(location.price)->second};
            level.totalQty -= quantity;
            location.pt->quantity -= quantity;
            break;
        }
        case Side::None:
        default:
            assert(false && " an order in the index has no side");
            return BookStatus::InvalidOrder;
    }
    return BookStatus::Ok;
}

BookStatus OrderBook::modify(std::int64_t id, std::int64_t quantity) {
    IndexIt ind = m_index.find(id);
    if (ind == m_index.end()) return BookStatus::UnknownId;
    if (quantity <= 0 ) return BookStatus::InvalidOrder;
    Location& location {ind->second};
    std::int64_t diffQty  {quantity - location.pt->quantity};

    switch (location.side) {
        case Side::Buy: {
            Level& level{m_bidSide.find(location.price)->second};
            level.totalQty += diffQty;
            location.pt->quantity = quantity;
            if (diffQty > 0 ) {
                level.queue.splice(level.queue.end(),level.queue , location.pt);
            }
            return BookStatus::Ok;
        }
        case Side::Sell:{
            Level& level{m_askSide.find(location.price)->second};
            level.totalQty += diffQty;
            location.pt->quantity = quantity;
            if (diffQty > 0 ) {
                level.queue.splice(level.queue.end(),level.queue , location.pt);
            }
            return BookStatus::Ok;
        }
        case Side::None:
        default:
            assert(false && " an order in the index has no side");
            return BookStatus::InvalidOrder;
    }

}