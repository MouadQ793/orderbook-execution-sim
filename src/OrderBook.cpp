//
// Created by mouad-sehli on 11/8/26.
//

#include "ob/OrderBook.h"
#include <cassert>
#include <iterator>
#include <iostream>

BookStatus OrderBook::add(const OrderEvent& event) {

    // if not add -> wrong function, if already in index -> duplicate, if <= qty -> Invalid
    if (event.type!= OrderType::Add) return BookStatus::InvalidOrder;
    if (m_index.contains(event.orderId)) return BookStatus::DuplicateId;
    if (event.quantity <= 0) return BookStatus::InvalidOrder;
    if (event.price <= 0) return BookStatus::InvalidOrder;
    if (event.side != Side::Buy && event.side != Side::Sell) return BookStatus::InvalidOrder;
    removeCrossedLevels(event.side,event.price); // stopping cross over from happening.

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

            // assigning order of execution
            Location location = {event.side, event.price, prevIt,this->m_tickets};
            this->m_tickets++;

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
            // order of executions
            Location location = {event.side, event.price, prevIt,this->m_tickets};
            this->m_tickets++;

            m_index.insert({event.orderId, location});
            return BookStatus::Ok;
        }
        case Side::None:
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
            // Checking if the quantity increases so we change the order of execution
            if (location.pt ->quantity < quantity ) {
                // if the new quantity is bigger move the order to the end of the queue, and change its ticket to the last position
                location.ticket = this->m_tickets;
                this->m_tickets++;
                level.queue.splice(level.queue.end(),level.queue , location.pt);
            }
            location.pt->quantity = quantity;
            return BookStatus::Ok;
        }
        case Side::Sell:{
            Level& level{m_askSide.find(location.price)->second};
            level.totalQty += diffQty;
            // Checking if the quantity increases so we change the order of execution
            if (location.pt ->quantity < quantity ) {
                // if the new quantity is bigger move the order to the end of the queue, and change its ticket to the last position
                location.ticket = this->m_tickets;
                this->m_tickets++;
                level.queue.splice(level.queue.end(),level.queue , location.pt);
            }
            location.pt->quantity = quantity;
            return BookStatus::Ok;
        }
        case Side::None:
        default:
            assert(false && " an order in the index has no side");
            return BookStatus::InvalidOrder;
    }

}

BookStatus OrderBook::apply(const OrderEvent& event) {

    switch (event.type) {
        case OrderType::Add:
            return add(event);
        case OrderType::Modify:
            return modify(event.orderId, event.quantity);
        case OrderType::Cancel:
            return cancel(event.orderId);
        case OrderType::Execute:
            return execute(event.orderId, event.quantity);
        case OrderType::None:
            return BookStatus::InvalidOrder;
        default:
            assert(false && " Apply : an order in the index has no type");
            return BookStatus::InvalidOrder;
    }

}

// the std::ostream param is to decite where to print
void OrderBook::print(std::ostream& out) const {
    // Bids: best (highest) price first, thanks to std::greater
    out << "Bids: ";
    if (m_bidSide.empty()) out << "empty";
    bool firstLevel{true};
    for (const auto& [price, level] : m_bidSide) {
        if (!firstLevel) out << ", ";
        firstLevel = false;
        out << price << " --> [";

        bool firstOrder{true};  // reset for each level
        for (const auto& rank : level.queue) {
            if (!firstOrder) out << ", ";
            firstOrder = false;
            out << '#' << rank.id << ':' << rank.quantity;
        }
        out << ']';
    }
    out << '\n';

    // Asks: best (lowest) price first
    out << "Asks: ";
    if (m_askSide.empty()) out << "empty";
    firstLevel = true;          // reset for the other side
    for (const auto& [price, level] : m_askSide) {
        if (!firstLevel) out << ", ";
        firstLevel = false;
        out << price << " --> [";

        bool firstOrder{true};  // reset for each level
        for (const auto& rank : level.queue) {
            if (!firstOrder) out << ", ";
            firstOrder = false;
            out << '#' << rank.id << ':' << rank.quantity;
        }
        out << ']';
    }
    out << '\n';
}

void OrderBook::removeCrossedLevels(Side side, std::int64_t price) {
    switch (side) {
        case Side::Buy:
            while (!m_askSide.empty() && m_askSide.begin()->first <= price) {
                auto levelIt{m_askSide.begin()};
                Level& level{levelIt->second};
                for (const Order& order : level.queue) {
                    m_index.erase(order.id);
                    ++m_staleRemoved;
                }
                m_askSide.erase(levelIt);
            }
            break;
        case Side::Sell:
            while (!m_bidSide.empty() && m_bidSide.begin()->first >= price) {
                auto levelIt{m_bidSide.begin()};
                Level& level{levelIt->second};
                for (const Order& order : level.queue) {
                    m_index.erase(order.id);
                    ++m_staleRemoved;
                }
                m_bidSide.erase(levelIt);
            }
            break;
        case Side::None:
        default:
            assert(false && " removeCrossedLevels: new order with no side");
    }
}

void OrderBook::printStaleRemoved() const {
    std::cout << " The number of lines removed because of a crossover is " << this->m_staleRemoved;
}

bool OrderBook::checkInvariants() const {
    // We check  that no invariants are broken eg
    // Total quantity is the sum of quantities in the list
    // no empty level is left
    // prices are mapped correctly
    // the indexes matches the queues
    // the side in location match side of book
    // number of orders in the books match total number of orders
    // book must never be crossed
    std::size_t orderCount{0};
    for (const auto& [price, level]: m_bidSide) {
        std::int64_t Qty{0};
        std::int64_t totalQty{level.totalQty};
        for (const Order& order : level.queue) {
            Qty += order.quantity;
            const auto ind{m_index.find(order.id)};
            if (ind==m_index.end()) return false;
            if (ind->second.side != Side::Buy) return false;
            if (&*ind->second.pt != &order ) return false;
            if (ind->second.price != price) return false;
            orderCount++;
        }
        if (level.price != price) return false;
        if (totalQty != Qty ) return false;
        if (level.queue.empty()) return false;
    }

    for (const auto& [price, level]: m_askSide) {
        std::int64_t Qty{0};
        std::int64_t totalQty{level.totalQty};
        for (const Order& order : level.queue) {
            Qty += order.quantity;
            const auto ind{m_index.find(order.id)};
            if (ind==m_index.end()) return false;
            if (ind->second.side != Side::Sell) return false;
            if (&*ind->second.pt != &order ) return false;
            if (ind->second.price != price) return false;
            orderCount++;
        }
        if (level.price != price) return false;
        if (totalQty != Qty ) return false;
        if (level.queue.empty()) return false;
    }
    if (orderCount != m_index.size()) return false;

    const std::optional<TopOfBook> bestbid{this->bestBid()};
    const std::optional<TopOfBook> bestask{this->bestAsk()};
    if (bestbid && bestask && bestbid->price >= bestask->price) return false;

    return true;
}


std::int64_t OrderBook::depth(Side side, std::int64_t k) const {
    // Computes depth of order book, goes through the levels summing their quantities until k levels have been considered
    std::int64_t Qty{0};
    int j{0};
    if (k <= 0) {std::cerr << "negative or null depth requested "; return 0;}
    switch (side) {
        case Side::Buy:
            for (const auto& [price, level]:m_bidSide) {
                Qty += level.totalQty;
                j++;
                if (j == k) break;
            }
            return Qty;
        case Side::Sell:
            for (const auto& [price, level]:m_askSide) {
                Qty += level.totalQty;
                j++;
                if (j == k) break;
            }
            return Qty;
        case Side::None:
        default:
            assert(false && " Error in computing imbalance");
            return 0;
    }
}


std::optional<std::int64_t> OrderBook::queueAhead(std::int64_t orderId) const {
    auto ind{this->m_index.find(orderId)};
    if (ind == this->m_index.end()) {/* std::cerr << "Queue Ahead received Unknown Id \n" ; */ return std::nullopt; }
    std::int64_t queueQty{0};
    Location location{ind->second};
    switch (location.side) {
        case Side::Buy: {
            const Level& level {m_bidSide.find(location.price)->second};
            for (auto ite{level.queue.begin()}; ite != location.pt; ++ite) {
                queueQty += ite->quantity;
            }
            return queueQty;
        }
        case Side::Sell: {
            const Level& level {m_askSide.find(location.price)->second};
            for (auto ite{level.queue.begin()}; ite != location.pt; ++ite) {
                queueQty += ite->quantity;
            }
            return queueQty;
        }
        case Side::None:
        default:
            assert(false && " Error in computing imbalance");
            return std::nullopt;
    }
}


std::optional<std::int64_t> OrderBook::getTicket(std::int64_t orderId) const {
    auto ind{this->m_index.find(orderId)};
    if (ind == m_index.end()) {std::cerr << "getTicket, Unkown ID"; return std::nullopt;}
    std::int64_t ticket{ind->second.ticket};
    return ticket;
}

std::optional<OrderInfo> OrderBook::getOrderInfo(std::int64_t orderId) const {
    // grabs all orderInfo to help
    const auto ind{this->m_index.find(orderId)};
    if (ind == this->m_index.end()) return std::nullopt;
    const Location& location{ind->second};
    OrderInfo orderInfo{location.ticket, location.price, location.pt->quantity, location.side};
    return orderInfo;
}











