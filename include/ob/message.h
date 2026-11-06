
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

enum class LobsterType : std::uint8_t {
    NewOrder      = 1,
    PartialCancel = 2,
    Delete        = 3,
    Execution     = 4,
    HiddenExecution = 5,
    CrossTrade     = 6,
    Halt          = 7,
};

struct LobsterEvent {
    std::int64_t timestamp{};
    LobsterType type{};
    std::int64_t orderId{};
    std::int64_t quantity{};
    std::int64_t price{};
    Side side{};
};