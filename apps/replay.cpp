#include <cassert>
#include "ob/OrderBook.h"
#include <unordered_map>
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
    int c{0};
    while (std::getline(testBook, orderInput)) {
        std::array<std::string_view, fieldCount> fields{splitOrder(orderInput)};
        std::optional<OrderEvent> event{fitOrderEvent(fields)};

        orderBook.apply(*event);
        std::optional<TopOfBook> bestbid{orderBook.bestBid()};
        std::optional<TopOfBook> bestask{orderBook.bestAsk()};

        if (bestbid && bestask && bestbid->price >= bestask->price) {
           c++;
        }
    }
    std::cout << c;
    orderBook.printStaleRemoved();

    return 0;
}