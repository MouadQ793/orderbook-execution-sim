#pragma once
#include <cstdint>


constexpr std::int64_t powint(std::int64_t base, int exp) {
    std::int64_t result{1};
    while (exp > 0) {
        if (exp & 1) result *= base;
        exp /= 2;
        base *= base;
    }
    return result;
}

constexpr std::int64_t nsPerSecond{powint(10, 9)};
constexpr std::int64_t openSeconds{34200};