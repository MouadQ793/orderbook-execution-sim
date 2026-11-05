//
// Created by mouad-sehli on 11/5/26.
//
#pragma once

#include <string>
#include <array>

inline constexpr std::size_t fieldOrder {6};
std::array<std::string_view, fieldOrder> splitOrder(std::string_view line);
void printArray(const std::array<std::string_view, fieldOrder>& order);

