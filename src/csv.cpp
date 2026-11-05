#include "ob/csv.h"
#include <iostream>

std::array<std::string_view, fieldOrder> splitOrder(std::string_view line) {

    std::size_t position{0};
    std::size_t start{0};
    std::array<std::string_view, fieldOrder> result{};

    for (std::size_t j{0} ; j < fieldOrder; ++j) {
        position = line.find(',', start);
        result[j] = line.substr(start, position - start);
        start = position + 1 ;
    }

    return result;
}

void printArray(const std::array<std::string_view, fieldOrder>& order) {
    for (std::size_t i {0}; i < order.size() ; ++i) {
        if ( i> 0 ) std::cout << " | ";
        std::cout << order[i];
    }
    std::cout << "\n";
}


