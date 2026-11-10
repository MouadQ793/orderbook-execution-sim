#include <cassert>
#include "ob/OrderBook.h"
#include <array>
#include <optional>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "ob/Indicators.h"
#include "ob/parsing.h"
#include "ob/QueueTracker.h"


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
    QueueTracker orderTracker{};
    std::string orderInput{};
    std::getline(LobsterBook, orderInput);   // header only in my tests
    std::int64_t lineNumber{};
    std::int64_t n{0}; // number of adds seen

    // queue tracker check: one add in trackEvery is tracked, compared with queueAhead after every message
    constexpr std::int64_t trackEvery{1000};
    std::vector<std::int64_t> trackedIds{};
    std::int64_t comparisons{0};
    std::int64_t mismatches{0};

    while (std::getline(LobsterBook, orderInput)) {
        lineNumber++;
        // Read and split the order  then fit it into an event
        std::array<std::string_view, fieldCount> fields{splitOrder(orderInput)};
        std::optional<OrderEvent> event{fitOrderEvent(fields)};

        if (!event) {
            continue;
            // avoids undefind behavior in case a line fails to parse
        }

        orderTracker.updateBeforeApply(*event, orderBook);
        // Apply the event in the orderbook, print it or not, compute indicators, and add them to the file
        orderBook.apply(*event);
        orderTracker.updateAfterApply(*event, orderBook);
        // orderBook.print(std::cout);
        indicators.update(orderBook, *event);
        indicatorsOutput << indicators.toCsvLine(event->timestamp);

        // start tracking one add in trackEvery (after apply, so the order is in the book)
        if (event->type == OrderType::Add) {
            n++;
            if (n % trackEvery == 0 && orderTracker.startTracking(event->orderId, orderBook)) {
                trackedIds.push_back(event->orderId);
            }
        }

        // compare the tracker with the exact walk for every order still tracked
        for (const std::int64_t id : trackedIds) {
            const auto trackedAhead{orderTracker.ahead(id)};
            if (!trackedAhead) continue; // order gone: filled, cancelled or uncrossed
            const auto exactAhead{orderBook.queueAhead(id)};
            comparisons++;
            if (!exactAhead || *exactAhead != *trackedAhead) {
                mismatches++;
                if (mismatches <= 5) {
                    std::cerr << "mismatch line " << lineNumber << " id " << id
                              << " tracker " << *trackedAhead
                              << " exact " << (exactAhead ? *exactAhead : -1) << '\n';
                }
            }
        }

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
    std::cout << "queue tracker: " << trackedIds.size() << " orders tracked, "
              << comparisons << " comparisons, " << mismatches << " mismatches\n";

    return 0;
}