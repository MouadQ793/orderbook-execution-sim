#include <cassert>
#include "ob/parsing.h"
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <string>

int main() {

    std::ifstream testBook{};
    std::ofstream convertedBook{"Data/testing/converted_book_lobster.csv"};
    testBook.open("Data/MSFT_2012-06-21_34200000_57600000_message_10.csv"); // explicitely opening book

    if (!testBook || !convertedBook) { // handle each error on its own later
        std::cerr << "Couldn't read the book, check Path " << std::endl;
        return 1;
    }

    // counters for the translated lines
    std::int64_t readLines{0},validLines{0},invalidLines{0},skippedLines{0},countA{0},countM{0},countC{0},countE{0};
    std::string orderInput{};
    QuantityMap qtyTracker{};

    //std::getline(testBook, orderInput); // to get out the first line

    convertedBook << "timestamp,type,order_id,side,price,quantity" << '\n';
    while (std::getline(testBook, orderInput)){
        // reads the events
        std::array<std::string_view, fieldCount> fields{splitOrder(orderInput)};
        readLines++;
        // parse
        std::optional<LobsterEvent> lobsterEvent{parseLobsterEvent(fields)};
        if (!lobsterEvent) {
            std::cerr << "Invalid line: " << orderInput << '\n';
            invalidLines++;
            continue;   // skip to the next line
        }

        std::optional<OrderEvent> order{translateLobsterEvent(qtyTracker,*lobsterEvent)};
        if (!order) {
            skippedLines++;
            continue;   // skip to the next line
        }
        // printOrder(*order); can use it to see printed orders, removed because slow
        convertedBook << toCsvLine(*order) << '\n';
        validLines++;
        switch (order->type) {
            case OrderType::Add: countA++; break;
            case OrderType::Modify: countM++; break;
            case OrderType::Cancel: countC++; break;
            case OrderType::Execute: countE++; break;
            default :
                // This should never happen because it should have been skipped in the prior checks
                assert(false && "Unkown order type");
                return 1;
        }
    }
    std::cout << "Read Lines " << readLines << '\n';
    std::cout << "Valid lines " << validLines << '\n';
    std::cout << "Invalid lines " << invalidLines << '\n';
    std::cout << "Skipped " << skippedLines << '\n';
    std::cout << "Number of add orders " << countA << '\n';
    std::cout << "Number of modify orders " << countM << '\n';
    std::cout << "Number of execute orders " << countE << '\n';
    std::cout << "Number of cancel orders " << countC << '\n';
    return 0;
}
