#include "ob/parsing.h"
#include <iostream>
#include <charconv>
#include <optional>
#include <system_error>
#include <unordered_map>

#include "ob/utils.h"

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

std::optional<std::int64_t> timeToNano(std::string_view originalTime) {
    // Finding the position where we split
    std::size_t position{originalTime.find('.',0)};
    std::int64_t time{0},base{0};

    // we extract the time in seconds before "." and check if it's valid or not because of a wrong input.
    std::optional<std::int64_t> seconds{parseInt(originalTime.substr(0,position))};
    if (!seconds.has_value() ) return std::nullopt;
    // if time is valid and  there is no dot return just base
    base = (*seconds - openSeconds) * nsPerSecond;
    if (position == std::string_view::npos) return base;

    // if there is a dot
    // we check if the parsing is valid and not typo
    // then if there are indeed 9 numbers for nano convention
    std::size_t checkNano{(originalTime.substr(position + 1 ,originalTime.size() - position )).size()};
    std::optional<std::int64_t> nanos{parseInt(originalTime.substr(position + 1,originalTime.size() - position ))};
    if (!nanos.has_value()) return std::nullopt;

    // if it's fine compute the power missing for nanos, add both and return time
    const int power{9 - static_cast<int>(checkNano)};
    if (checkNano <9) *nanos = *nanos * powint(10, power);
    time = base + *nanos;

    return time;
}


std::optional<LobsterEvent> parseLobsterEvent(const std::array<std::string_view,fieldCount>& order) {
    LobsterEvent event{};

    // loading time

    std::string_view lobsterTime{order[0]};
    std::optional<std::int64_t> time{timeToNano(lobsterTime)};
    if (!time) return std::nullopt;
    event.timestamp = *time;

    // loading type

    std::optional<std::int64_t> type{parseInt(order[1])};
    if (!type.has_value()) return std::nullopt;
    switch (*type) {
        case 1:
            event.type = LobsterType::NewOrder;
            break;
        case 2:
            event.type = LobsterType::PartialCancel;
            break;
        case 3:
            event.type = LobsterType::Delete;
            break;
        case 4:
            event.type = LobsterType::Execution;
            break;
        case 5:
            event.type = LobsterType::HiddenExecution;
            break;
        case 6:
            event.type = LobsterType::CrossTrade;
            break;
        case 7:
            event.type = LobsterType::Halt;
            break;
        default:
            return std::nullopt;
    }

    // Loading order ID
    std::optional<std::int64_t> orderId{parseInt(order[2])};
    if (!orderId.has_value()) return std::nullopt;
    event.orderId = *orderId;

    // Loading quantity
    std::optional<std::int64_t> size{parseInt(order[3])};
    if (!size.has_value()) return std::nullopt;
    event.quantity = *size;

    // loading price
    std::optional<std::int64_t> price{parseInt(order[4])};
    if (!price.has_value()) return std::nullopt;
    if (*price % 100 == 0 ) event.price = *price/100;
    else return std::nullopt;

    // loading side
    std::optional<std::int64_t> side{parseInt(order[5])};
    if (!side.has_value()) return std::nullopt;
    switch (*side) {
        case 1 : event.side = Side::Buy; break;
        case -1 : event.side = Side::Sell; break;
        default: return std::nullopt;
    }

    return event;
}


std::optional<OrderEvent> translateLobsterEvent(std::unordered_map<std::int64_t, std::int64_t>& qtyTracker,const LobsterEvent& lobsterEvent) {
    OrderEvent orderEvent{};
    // These traits don't depend on type of order
    orderEvent.timestamp = lobsterEvent.timestamp;
    orderEvent.orderId = lobsterEvent.orderId;


    // if the order isn't a new one and there is no history of it, skip
    if (lobsterEvent.type != LobsterType::NewOrder && !qtyTracker.contains(lobsterEvent.orderId)) {
        return std::nullopt;
    }
    switch (lobsterEvent.type) {
        case LobsterType::NewOrder: {
            qtyTracker[lobsterEvent.orderId] = lobsterEvent.quantity;
            orderEvent.type = OrderType::Add ;
            orderEvent.side = lobsterEvent.side;
            orderEvent.price = lobsterEvent.price;
            orderEvent.quantity = lobsterEvent.quantity;
            break;
        }
        case LobsterType::Delete: qtyTracker.erase(lobsterEvent.orderId); orderEvent.type = OrderType::Cancel ; break;
        case LobsterType::PartialCancel:
        case LobsterType::Execution: {
            // computing remained once as const bcs of bug in Modify that would recreate the map in case of 0
            // If remainder in order >0 keep else delete from tracker, regardless put in translated event
            const std::int64_t remaining{qtyTracker[lobsterEvent.orderId] - lobsterEvent.quantity};
            if (remaining > 0)
                qtyTracker[lobsterEvent.orderId] = remaining;
            else qtyTracker.erase(lobsterEvent.orderId);

            if (lobsterEvent.type == LobsterType::PartialCancel) {
                orderEvent.type = OrderType::Modify;
                orderEvent.quantity = remaining;
            }
            if (lobsterEvent.type == LobsterType::Execution) {
                orderEvent.type = OrderType::Execute;
                orderEvent.quantity = lobsterEvent.quantity;
            }
            break;
        }
        case LobsterType::CrossTrade:;
        case LobsterType::Halt:;
        case LobsterType::HiddenExecution:;
        default: return std::nullopt;
    }

    return orderEvent;
}