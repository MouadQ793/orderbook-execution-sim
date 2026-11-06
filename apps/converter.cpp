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
        std::array<std::string_view, fieldCount> fields{splitOrder(orderInput)};

        // Step 2: turn the fields into an OrderEvent (or nothing, if the line is invalid)
        std::optional<OrderEvent> event{fitOrderEvent(fields)};

    }
    return 0;
}