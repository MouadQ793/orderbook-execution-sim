#include <cassert>
#include "ob/OrderBook.h"
#include <array>
#include <optional>
#include <iostream>
#include <fstream>
#include <string>

#include "ob/Indicators.h"
#include "ob/parsing.h"


int main() {
    // taking the book as input file and creating output file for indicators
    std::ifstream LobsterBook{};
    std::ofstream indicatorsOutput{"Data/indicators_msft.csv"};
    LobsterBook.open("Data/converted_book_lobster.csv");
    // writting the header
    indicatorsOutput << Indicators::csvHeader << '\n';



    if (!LobsterBook) {
        std::cerr << "Couldn't read the path check book" << '\n';
        return 1;
    }

    // Initializing my stuctures
    OrderBook orderBook{};
    Indicators indicators{};
    std::string orderInput{};
    std::getline(LobsterBook, orderInput);   // header only in my tests
    std::int64_t lineNumber{};


    while (std::getline(LobsterBook, orderInput)) {
        lineNumber++;
        // Read and split the order  then fit it into an event
        std::array<std::string_view, fieldCount> fields{splitOrder(orderInput)};
        std::optional<OrderEvent> event{fitOrderEvent(fields)};

        if (!event) {
            continue;
            // avoids undefind behavior in case a line fails to parse
        }
        // Apply the event in the orderbook, print it or not, compute indicators, and add them to the file
        orderBook.apply(*event);
        // orderBook.print(std::cout);
        indicators.update(orderBook, *event);
        indicatorsOutput << indicators.toCsvLine(event->timestamp) << '\n';

        // checking invariants punctually and then at last line (next loop)
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