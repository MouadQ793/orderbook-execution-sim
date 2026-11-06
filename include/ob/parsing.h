//
// Created by mouad-sehli on 11/5/26.
//
#pragma once

#include "ob/message.h"
#include <string_view>
#include <array>
#include <cstddef>
#include <optional>


inline constexpr std::size_t fieldCount {6};
std::array<std::string_view, fieldCount> splitOrder(std::string_view line);
void printArray(const std::array<std::string_view, fieldCount>& order);
std::optional<OrderEvent> fitOrderEvent(const std::array<std::string_view, fieldCount>& order);