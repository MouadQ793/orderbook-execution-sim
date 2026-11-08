#include <gtest/gtest.h>
#include "ob/OrderBook.h"



// Test of function to ADD
TEST(OrderBook, EmptyBookHasNoBestPrices) {
    OrderBook book{};
    const std::optional<TopOfBook> bid{book.bestBid()};
    const std::optional<TopOfBook> ask{book.bestAsk()};

    ASSERT_FALSE(ask.has_value());
    ASSERT_FALSE(bid.has_value());
}

TEST(OrderBook, AddBuySetsBestBid) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 50}),BookStatus::Ok);

    const std::optional<TopOfBook> bid{book.bestBid()};
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->price, 100);
    EXPECT_EQ(bid->totalQty, 50);
}

TEST(OrderBook, AddSellSetsBestAsk) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 50}),BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 2 , Side::Sell, 103, 30}),BookStatus::Ok);
    const std::optional<TopOfBook> ask{book.bestAsk()};
    ASSERT_TRUE(ask.has_value());
    EXPECT_EQ(ask->price, 103);
    EXPECT_EQ(ask->totalQty, 30);
}

TEST(OrderBook, SamePriceAddsToTotal) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 50}),BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 2 , Side::Buy, 100, 30}),BookStatus::Ok);

    const std::optional<TopOfBook> bid{book.bestBid()};
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->price, 100);
    EXPECT_EQ(bid->totalQty, 80);
}

TEST(OrderBook, BetterBidBecomesBest) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 50}),BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 2 , Side::Buy, 100, 30}),BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 3 , Side::Buy, 105, 15}),BookStatus::Ok);

    const std::optional<TopOfBook> bid{book.bestBid()};
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->price, 105);
    EXPECT_EQ(bid->totalQty, 15);
}

TEST(OrderBook, WorseAskDoesNotChangeBest) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 50}),BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 2 , Side::Sell, 103, 30}),BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 3 , Side::Sell, 105, 200}),BookStatus::Ok);

    const std::optional<TopOfBook> ask{book.bestAsk()};
    ASSERT_TRUE(ask.has_value());
    EXPECT_EQ(ask->price, 103);
    EXPECT_EQ(ask->totalQty, 30);
}

TEST(OrderBook, DuplicateIdRejected) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 50}),BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Sell, 103, 30}),BookStatus::DuplicateId);
    const std::optional<TopOfBook> ask{book.bestAsk()};
    const std::optional<TopOfBook> bid{book.bestBid()};

    // making sure that the ask didn't get created before returning error  and the bid didn't change
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->price, 100);
    EXPECT_EQ(bid->totalQty, 50);
    ASSERT_FALSE(ask.has_value());
}

TEST(OrderBook, InvalidOrdersRejected) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 0}),BookStatus::InvalidOrder);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, -7}),BookStatus::InvalidOrder);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, -4, 30}),BookStatus::InvalidOrder);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 0, 70}),BookStatus::InvalidOrder);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Modify, 1 , Side::Buy, 100, 10}),BookStatus::InvalidOrder);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::None, 100, 10}),BookStatus::InvalidOrder);

    const std::optional<TopOfBook> ask{book.bestAsk()};
    const std::optional<TopOfBook> bid{book.bestBid()};
    ASSERT_FALSE(ask.has_value());
    ASSERT_FALSE(bid.has_value());
}



// Test of function to cancel


TEST(OrderBook, CancelOnlyBuyEmptiesBidSide) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 50}),BookStatus::Ok);
    EXPECT_EQ(book.cancel(1),BookStatus::Ok);

    const std::optional<TopOfBook> bid{book.bestBid()};
    ASSERT_FALSE(bid.has_value());
}

TEST(OrderBook, CancelOnlySellEmptiesAskSide) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 50}),BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 2 , Side::Sell, 103, 150}),BookStatus::Ok);
    EXPECT_EQ(book.cancel(2),BookStatus::Ok);

    const std::optional<TopOfBook> ask{book.bestAsk()};
    const std::optional<TopOfBook> bid{book.bestBid()};
    ASSERT_TRUE(bid.has_value());
    ASSERT_FALSE(ask.has_value());
}

TEST(OrderBook, CancelOneOfTwoKeepsLevel) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 50}),BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 2 , Side::Buy, 100, 140}),BookStatus::Ok);
    EXPECT_EQ(book.cancel(1),BookStatus::Ok);

    const std::optional<TopOfBook> bid{book.bestBid()};

    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->price, 100);
    EXPECT_EQ(bid->totalQty, 140);
}

TEST(OrderBook, CancelBestLevelMovesBestBid) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 50}),BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 2 , Side::Buy, 99, 140}),BookStatus::Ok);
    EXPECT_EQ(book.cancel(1),BookStatus::Ok);

    const std::optional<TopOfBook> ask{book.bestAsk()};
    const std::optional<TopOfBook> bid{book.bestBid()};
    ASSERT_TRUE(bid.has_value());
    ASSERT_FALSE(ask.has_value());
    EXPECT_EQ(bid->price, 99);
    EXPECT_EQ(bid->totalQty, 140);
}

TEST(OrderBook, CancelUnknownIdChangesNothing) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 50}),BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 2 , Side::Sell, 103, 140}),BookStatus::Ok);
    EXPECT_EQ(book.cancel(7),BookStatus::UnknownId);

    const std::optional<TopOfBook> ask{book.bestAsk()};
    const std::optional<TopOfBook> bid{book.bestBid()};
    ASSERT_TRUE(bid.has_value());
    ASSERT_TRUE(ask.has_value());
    EXPECT_EQ(bid->price, 100);
    EXPECT_EQ(ask->price, 103);
    EXPECT_EQ(bid->totalQty, 50);
    EXPECT_EQ(ask->totalQty, 140);
}

TEST(OrderBook, CancelTwiceRejectsSecond) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 50}),BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 2 , Side::Sell, 103, 140}),BookStatus::Ok);
    EXPECT_EQ(book.cancel(2),BookStatus::Ok);
    EXPECT_EQ(book.cancel(2),BookStatus::UnknownId);

    const std::optional<TopOfBook> ask{book.bestAsk()};
    const std::optional<TopOfBook> bid{book.bestBid()};
    ASSERT_TRUE(bid.has_value());
    ASSERT_FALSE(ask.has_value());
    EXPECT_EQ(bid->price, 100);
}


































