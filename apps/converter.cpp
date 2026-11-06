#include "ob/parsing.h"

#include <iostream>
#include <fstream>
#include <string>

int main() {
    std::ifstream testBook{};
    testBook.open("Data/test_small.csv"); // explicitely opening book

    if (!testBook) {
        std::cerr << "Couldn't read the book, check Path " << std::endl;
    }

    std::string orderInput{};
    std::getline(testBook, orderInput);

    while (std::getline(testBook, orderInput)){
        // reads the events
        std::array<std::string_view, fieldCount> fields{splitOrder(orderInput)};

        // Turn the 6 fields into an orderevent element
        std::optional<OrderEvent> event{fitOrderEvent(fields)};

        // Printing the order event
        printOrder(*event);
    }
    return 0;
}
