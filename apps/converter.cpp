#include "ob/csv.h"

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
        printArray(splitOrder(orderInput));
    }
    return 0;
}