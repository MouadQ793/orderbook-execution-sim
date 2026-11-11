#include <gtest/gtest.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include "ob/Engine.h"
#include "ob/OrderBook.h"

// Each test writes its own small input file in the system temp folder, so the tests
// don't depend on the folder GoogleTest is run from.

namespace {
    const std::string header{"timestamp,type,order_id,side,price,quantity"};

    std::string writeInput(const std::string& name, const std::string& lines) {
        const std::filesystem::path path{std::filesystem::temp_directory_path() / name};
        std::ofstream out{path};
        out << header << '\n' << lines;
        return path.string();
    }

    void expectTop(const std::optional<TopOfBook>& top, std::int64_t price, std::int64_t qty) {
        ASSERT_TRUE(top.has_value());
        EXPECT_EQ(top->price, price);
        EXPECT_EQ(top->totalQty, qty);
    }
}

TEST(Engine, ReplaysSmallFile) {
    const std::string path{writeInput("engine_replay.csv",
        "10,A,1,B,100,50\n"
        "20,A,2,B,100,30\n"
        "30,A,3,S,102,40\n"
        "40,A,4,S,101,20\n"
        "50,C,2,,,\n"          // bid 100: 80 -> 50
        "60,E,4,,,20\n")};     // ask 101 fully executed: best ask back to 102
    Engine engine{path};
    engine.run();

    expectTop(engine.getOrderBook().bestBid(), 100, 50);
    expectTop(engine.getOrderBook().bestAsk(), 102, 40);
    EXPECT_TRUE(engine.getOrderBook().checkInvariants());
}

TEST(Engine, SameBookAsApplyingDirectly) {
    const std::string lines{
        "10,A,1,B,100,50\n"
        "10,A,2,S,103,30\n"     // same timestamp as the line before: file order must be kept
        "20,M,1,,,20\n"
        "30,A,3,B,101,10\n"
        "40,E,2,,,5\n"
        "50,A,4,S,103,70\n"};
    Engine engine{writeInput("engine_same.csv", lines)};
    engine.run();

    OrderBook reference{};
    reference.apply(OrderEvent{10, OrderType::Add, 1, Side::Buy, 100, 50});
    reference.apply(OrderEvent{10, OrderType::Add, 2, Side::Sell, 103, 30});
    reference.apply(OrderEvent{20, OrderType::Modify, 1, Side::None, 0, 20});
    reference.apply(OrderEvent{30, OrderType::Add, 3, Side::Buy, 101, 10});
    reference.apply(OrderEvent{40, OrderType::Execute, 2, Side::None, 0, 5});
    reference.apply(OrderEvent{50, OrderType::Add, 4, Side::Sell, 103, 70});

    const OrderBook& book{engine.getOrderBook()};
    expectTop(book.bestBid(), reference.bestBid()->price, reference.bestBid()->totalQty);
    expectTop(book.bestAsk(), reference.bestAsk()->price, reference.bestAsk()->totalQty);
    EXPECT_EQ(book.depth(Side::Buy, 5), reference.depth(Side::Buy, 5));
    EXPECT_EQ(book.depth(Side::Sell, 5), reference.depth(Side::Sell, 5));
    EXPECT_EQ(book.queueAhead(4), reference.queueAhead(4));   // 103: order 2 (25 left) then order 4
    EXPECT_TRUE(book.checkInvariants());
}

TEST(Engine, ClockEndsAtLastEvent) {
    Engine engine{writeInput("engine_clock.csv",
        "100,A,1,B,100,10\n"
        "250,A,2,S,101,10\n"
        "900,C,1,,,\n")};
    EXPECT_EQ(engine.getCurrentTime(), 0);   // nothing processed before run()
    engine.run();
    EXPECT_EQ(engine.getCurrentTime(), 900);
}

TEST(Engine, BadLinesAreSkipped) {
    Engine engine{writeInput("engine_bad.csv",
        "10,A,1,B,100,10\n"
        "this is not an order\n"
        "20,Z,2,,,\n"               // unknown type
        "30,A,3,S,abc,10\n"         // price not a number
        "40,A,4,S,101,25\n")};
    engine.run();

    expectTop(engine.getOrderBook().bestBid(), 100, 10);
    expectTop(engine.getOrderBook().bestAsk(), 101, 25);
    EXPECT_EQ(engine.getCurrentTime(), 40);
    EXPECT_TRUE(engine.getOrderBook().checkInvariants());
}

TEST(Engine, HeaderOnlyFileDoesNothing) {
    Engine engine{writeInput("engine_empty.csv", "")};
    engine.run();
    EXPECT_FALSE(engine.getOrderBook().bestBid().has_value());
    EXPECT_FALSE(engine.getOrderBook().bestAsk().has_value());
    EXPECT_EQ(engine.getCurrentTime(), 0);
}

TEST(Engine, MissingFileDoesNothing) {
    Engine engine{"this/file/does/not/exist.csv"};
    engine.run();                                   // must return at once, no crash
    EXPECT_FALSE(engine.getOrderBook().bestBid().has_value());
    EXPECT_FALSE(engine.getOrderBook().bestAsk().has_value());
    EXPECT_EQ(engine.getCurrentTime(), 0);
}