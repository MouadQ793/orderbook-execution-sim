#include <cassert>
#include "ob/OrderBook.h"
#include <array>
#include <optional>
#include <iostream>
#include <fstream>
#include <string>

#include "ob/parsing.h"


int main() {
    std::ifstream testBook{};
    OrderBook orderBook{};
    testBook.open("Data/converted_book_lobster.csv");

    if (!testBook) {
        std::cerr << "Couldn't read the path check book" << '\n';
        return 1;
    }

    std::string orderInput{};
    std::getline(testBook, orderInput);   // header
    std::int64_t lineNumber{};

    while (std::getline(testBook, orderInput)) {
        lineNumber++;
        std::array<std::string_view, fieldCount> fields{splitOrder(orderInput)};
        std::optional<OrderEvent> event{fitOrderEvent(fields)};
        if (!event) {
            continue;
            // avoids undefind behavior in case a line fails to parse
        }
        orderBook.apply(*event);

        if (lineNumber % 10000 == 0 && !orderBook.checkInvariants()) {
            std::cerr << "invariant broken at line " << lineNumber << '\n';
            break;
        }
    }
    if (!orderBook.checkInvariants()) {
        std::cerr << "invariant broken at end of file\n";
        return 1;
    }
    std::cout << "invariants OK: " << lineNumber << " lines, " << lineNumber / 10000 << " checks + final\n";

    return 0;
}