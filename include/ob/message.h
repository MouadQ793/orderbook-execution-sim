
#pragma once
#include <cstdint>

enum class OrderType : std::uint8_t {
    None,
    Add,
    Cancel,
    Execute,
    Modify,
};

enum class Side : std::uint8_t {
    None,
    Buy,
    Sell,
};

struct OrderEvent {
    std::int64_t timestamp{};
    OrderType type{};
    std::int64_t orderId{};
    Side side{};
    std::int64_t price{};
    std::int64_t quantity{};
};
