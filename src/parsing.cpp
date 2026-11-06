#include "ob/parsing.h"
#include <iostream>
#include <charconv>
#include <optional>
#include <system_error>



std::array<std::string_view, fieldCount> splitOrder(std::string_view line) {

    std::size_t position{0};
    std::size_t start{0};
    std::array<std::string_view, fieldCount> result{};

    for (std::size_t j{0} ; j < fieldCount; ++j) {
        position = line.find(',', start);
        result[j] = line.substr(start, position - start);
        start = position + 1 ;
    }

    return result;
}

// helper function to grab the numbers using pointers of location
namespace {
    std::optional<std::int64_t> parseInt(std::string_view field) {
        std::int64_t value{};
        const char* end{field.data() + field.size()};
        auto [ptr, ec] = std::from_chars(field.data(), end, value);
        if (ec != std::errc{} || ptr != end) {
            return std::nullopt;   // not a number, empty, too big, or extra characters
        }
        return value;
    }
}

std::optional<OrderEvent> fitOrderEvent(const std::array<std::string_view, fieldCount>& order) {

    OrderEvent event{};

    // grabing the numbers using the helper function
    std::optional<std::int64_t> timestamp{parseInt(order[0])};
    std::optional<std::int64_t> orderId{parseInt(order[2])};
    std::optional<std::int64_t> price{parseInt(order[4])};
    std::optional<std::int64_t> quantity{parseInt(order[5])};
    if (!timestamp || !orderId) {
        return std::nullopt;       // invalid timestamp or orderId: the whole line is invalid
    }

    // Here I just assign the values that are common among all type of orders
    event.timestamp = *timestamp;
    event.orderId = *orderId;

    if (order[1].size() != 1) return std::nullopt;

    switch (order[1][0]) {
        case 'A':
            if (!price || !quantity) return std::nullopt; // error handling in case of unvalid new order

            event.type = OrderType::Add;
            event.price = *price;
            event.quantity = *quantity;
            if (order[3].size() != 1) return std::nullopt;
            switch (order[3][0]) {
                case 'B': event.side = Side::Buy; break;
                case 'S': event.side = Side::Sell; break;
                default : return std::nullopt;
            }
            break;

        case 'C':
            event.type = OrderType::Cancel;
            break;

        case 'E':
            if (!quantity) return std::nullopt;
            event.type = OrderType::Execute;
            event.quantity = *quantity;
            break;

        case 'M':
            if (!quantity) return std::nullopt;
            event.type = OrderType::Modify;
            event.quantity = *quantity;
            break;

        default:
            return std::nullopt;
    }

    return event;
}

std::string_view toString(OrderType type) {
    switch (type) {
        case OrderType::Add:     return "ADD";
        case OrderType::Cancel:  return "CANCEL";
        case OrderType::Execute: return "EXECUTE";
        case OrderType::Modify:  return "MODIFY";
        default:                 return "NONE";
    }
}

std::string_view toString(Side side) {
    switch (side) {
        case Side::Buy: return "BUY";
        case Side::Sell: return "SELL";
        default: return "NONE";
    }
}

void printOrder(const OrderEvent& event) {
    std::cout << event.timestamp << '|';
    std::cout << toString(event.type) << '|';
    std::cout << event.orderId << '|';
    std::cout << toString(event.side) << '|';
    std::cout << event.price << '|';
    std::cout << event.quantity << '\n';
}