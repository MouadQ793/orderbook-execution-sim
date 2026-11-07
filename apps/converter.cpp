#include "ob/parsing.h"
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <string>

int main() {

    std::ifstream testBook{};
    std::ofstream convertedBook{"Data/testing/lobster_first12_converted.csv"};
    testBook.open("Data/testing/lobster_first12.csv"); // explicitely opening book

    if (!testBook || !convertedBook) { // handle each error on its own later
        std::cerr << "Couldn't read the book, check Path " << std::endl;
        return 1;
    }

    std::string orderInput{};
    QuantityMap qtyTracker{};

    //std::getline(testBook, orderInput); // to get out the first line

    convertedBook << "timestamp,type,order_id,side,price,quantity" << '\n';
    while (std::getline(testBook, orderInput)){
        // reads the events
        std::array<std::string_view, fieldCount> fields{splitOrder(orderInput)};

        // parse
        std::optional<LobsterEvent> lobsterEvent{parseLobsterEvent(fields)};
        if (!lobsterEvent) {
            std::cerr << "Invalid line: " << orderInput << '\n';
            continue;   // skip to the next line
        }

        std::optional<OrderEvent> order{translateLobsterEvent(qtyTracker,*lobsterEvent)};
        if (!order) {
            std::cerr << "Invalid line: " << orderInput << '\n';
            continue;   // skip to the next line
        }
        printOrder(*order);
        convertedBook << toCsvLine(*order) << '\n';
    }
    return 0;
}
