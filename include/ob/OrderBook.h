//
// Created by mouad-sehli on 10/8/26.
//

#pragma once
#include <cstdint>
#include <list>
#include <unordered_map>
#include <map>
#include "message.h"
#include <functional>
#include <iosfwd>
#include <optional>


// Enum to be able to determine why a new order would fail
enum class BookStatus : std::uint8_t {
    Ok,
    DuplicateId,
    UnknownId,
    InvalidOrder,
    ExcessQuantity,
};

struct TopOfBook {
    std::int64_t price{};
    std::int64_t totalQty{};
};

class OrderBook {

public:

    BookStatus add(const OrderEvent& event);
    BookStatus cancel(std::int64_t id);
    BookStatus modify(std::int64_t id, std::int64_t quantity);
    BookStatus execute(std::int64_t id, std::int64_t quantity);
    BookStatus apply(const OrderEvent& event);

    // getting the best offer for either sides, const because they only read the book
    std::optional<TopOfBook> bestBid() const;
    std::optional<TopOfBook> bestAsk() const;

    // function to print state of the book
    void print(std::ostream& out) const;
    void removeCrossedLevels(Side side, std::int64_t price);
    void printStaleRemoved() const;
    std::int64_t staleRemoved() const { return m_staleRemoved; }

private:
    std::int64_t m_staleRemoved{};

    struct Order {
        std::int64_t id{};
        std::int64_t quantity{};
    };

    struct Level {
        std::int64_t price{};
        std::int64_t totalQty{};
        std::list<Order> queue{};
    };
    struct Location {
        Side side{};
        std::int64_t price{};
        std::list<Order>::iterator pt{};
    };

    std::map<std::int64_t, Level, std::greater<std::int64_t>> m_bidSide{}; // std::greater tells the map to rank biggest to small on the int64_t param
    std::map<std::int64_t, Level> m_askSide{};
    std::unordered_map<std::int64_t, Location> m_index;
    using IndexIt = std::unordered_map<std::int64_t, Location>::iterator; // to make the helper function more readable
    void removeOrder(IndexIt ind);
};
