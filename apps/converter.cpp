#include "ob/parsing.h"
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <string>

int main() {

    std::ifstream testBook{};
    testBook.open("Data/MSFT_2012-06-21_34200000_57600000_message_10.csv"); // explicitely opening book

    if (!testBook) {
        std::cerr << "Couldn't read the book, check Path " << std::endl;
        return 1;
    }

    std::string orderInput{};
    QuantityMap qtyTracker{};

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
    }
    return 0;
}
