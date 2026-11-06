//
// Created by mouad-sehli on 11/5/26.
//
#pragma once

#include "ob/message.h"
#include "ob/utils.h"
#include <string_view>
#include <array>
#include <cstddef>
#include <optional>
#include <unordered_map>

using QuantityMap = std::unordered_map<std::int64_t, std::int64_t>;
inline constexpr std::size_t fieldCount {6};
std::array<std::string_view, fieldCount> splitOrder(std::string_view line);
void printArray(const std::array<std::string_view, fieldCount>& order);
std::optional<OrderEvent> fitOrderEvent(const std::array<std::string_view, fieldCount>& order);
std::string_view toString(OrderType type);
std::string_view toString(Side side);
void printOrder(const OrderEvent& event);
std::optional<std::int64_t> timeToNano(std::string_view originalTime);
std::optional<LobsterEvent> parseLobsterEvent(const std::array<std::string_view,fieldCount>& order);
std::optional<OrderEvent> translateLobsterEvent(QuantityMap& qtyTracker, const LobsterEvent& lobsterEvent);
