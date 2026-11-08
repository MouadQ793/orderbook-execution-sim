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

    std::int64_t lineNumber{1};           // the header was line 1
    std::int64_t notOk{0};

    while (std::getline(testBook, orderInput)) {
        ++lineNumber;
        std::array<std::string_view, fieldCount> fields{splitOrder(orderInput)};
        std::optional<OrderEvent> event{fitOrderEvent(fields)};

        if (!event) {
            std::cerr << "Invalid line " << lineNumber << ": " << orderInput << '\n';
            continue;
        }

        const BookStatus status{orderBook.apply(*event)};
        if (status != BookStatus::Ok) {
            ++notOk;
            if (notOk <= 10) {   // show only the first few
                std::cout << "Rejected at line " << lineNumber << ": " << orderInput
                          << " (status " << static_cast<int>(status) << ")\n";
            }
        }


        // stop at the first moment the book is crossed
        const std::optional<TopOfBook> bid{orderBook.bestBid()};
        const std::optional<TopOfBook> ask{orderBook.bestAsk()};
        if (bid && ask && bid->price >= ask->price) {
            std::cout << "Book crossed at line " << lineNumber << ": " << orderInput << '\n';
            orderBook.print(std::cout);
            break;
        }
    }

    std::cout << "Lines read: " << lineNumber << ", rejected by the book: " << notOk << '\n';
    return 0;
}