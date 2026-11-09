#include <gtest/gtest.h>
#include "ob/OrderBook.h"



// Test of function to ADD
TEST(OrderBook, EmptyBookHasNoBestPrices) {
    OrderBook book{};
    const std::optional<TopOfBook> bid{book.bestBid()};
    const std::optional<TopOfBook> ask{book.bestAsk()};

    ASSERT_FALSE(ask.has_value());
    ASSERT_FALSE(bid.has_value());
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBook, AddBuySetsBestBid) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 50}),BookStatus::Ok);

    const std::optional<TopOfBook> bid{book.bestBid()};
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->price, 100);
    EXPECT_EQ(bid->totalQty, 50);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBook, AddSellSetsBestAsk) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 50}),BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 2 , Side::Sell, 103, 30}),BookStatus::Ok);
    const std::optional<TopOfBook> ask{book.bestAsk()};
    ASSERT_TRUE(ask.has_value());
    EXPECT_EQ(ask->price, 103);
    EXPECT_EQ(ask->totalQty, 30);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBook, SamePriceAddsToTotal) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 50}),BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 2 , Side::Buy, 100, 30}),BookStatus::Ok);

    const std::optional<TopOfBook> bid{book.bestBid()};
    ASSERT_TRUE(bid.has_value());
    EXPECT_EQ(bid->price, 100);
    EXPECT_EQ(bid->totalQty, 80);
    EXPECT_TRUE(book.checkInvariants());
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
    EXPECT_TRUE(book.checkInvariants());
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
    EXPECT_TRUE(book.checkInvariants());
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
    EXPECT_TRUE(book.checkInvariants());
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
    EXPECT_TRUE(book.checkInvariants());
}



// Test of function to cancel


TEST(OrderBook, CancelOnlyBuyEmptiesBidSide) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1 , Side::Buy, 100, 50}),BookStatus::Ok);
    EXPECT_EQ(book.cancel(1),BookStatus::Ok);

    const std::optional<TopOfBook> bid{book.bestBid()};
    ASSERT_FALSE(bid.has_value());
    EXPECT_TRUE(book.checkInvariants());
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
    EXPECT_TRUE(book.checkInvariants());
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
    EXPECT_TRUE(book.checkInvariants());
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
    EXPECT_TRUE(book.checkInvariants());
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
    EXPECT_TRUE(book.checkInvariants());
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
    EXPECT_TRUE(book.checkInvariants());
}


// Test of function to Execute

void expectTop(const std::optional<TopOfBook>& top, std::int64_t price, std::int64_t qty) {
    ASSERT_TRUE(top.has_value());
    EXPECT_EQ(top->price, price);
    EXPECT_EQ(top->totalQty, qty);
}

TEST(OrderBook, ExecutePartialReducesOrderAndLevel) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1, Side::Buy, 100, 50}), BookStatus::Ok);
    EXPECT_EQ(book.execute(1, 30), BookStatus::Ok);
    expectTop(book.bestBid(), 100, 20);

    // the order itself must now hold 20: executing 21 is too much, 20 empties it
    EXPECT_EQ(book.execute(1, 21), BookStatus::ExcessQuantity);
    EXPECT_EQ(book.execute(1, 20), BookStatus::Ok);
    EXPECT_FALSE(book.bestBid().has_value());
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBook, ExecuteFullRemovesOnlyOrderAndLevel) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1, Side::Sell, 102, 150}), BookStatus::Ok);
    EXPECT_EQ(book.execute(1, 150), BookStatus::Ok);
    EXPECT_FALSE(book.bestAsk().has_value());
    EXPECT_EQ(book.cancel(1), BookStatus::UnknownId);   // also gone from the index
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBook, ExecuteFullFirstOfTwoKeepsLevel) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1, Side::Sell, 105, 200}), BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 2, Side::Sell, 105, 120}), BookStatus::Ok);
    EXPECT_EQ(book.execute(1, 200), BookStatus::Ok);
    expectTop(book.bestAsk(), 105, 120);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBook, ExecuteUnknownIdChangesNothing) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1, Side::Buy, 100, 50}), BookStatus::Ok);
    EXPECT_EQ(book.execute(42, 10), BookStatus::UnknownId);
    expectTop(book.bestBid(), 100, 50);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBook, ExecuteExcessOrInvalidChangesNothing) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1, Side::Buy, 100, 50}), BookStatus::Ok);
    EXPECT_EQ(book.execute(1, 51), BookStatus::ExcessQuantity);
    EXPECT_EQ(book.execute(1, 0), BookStatus::InvalidOrder);
    EXPECT_EQ(book.execute(1, -5), BookStatus::InvalidOrder);
    expectTop(book.bestBid(), 100, 50);
    EXPECT_TRUE(book.checkInvariants());
}


// Test for modify

TEST(OrderBook, ModifyDecreaseReducesTotal) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1, Side::Buy, 100, 50}), BookStatus::Ok);
    EXPECT_EQ(book.modify(1, 30), BookStatus::Ok);
    expectTop(book.bestBid(), 100, 30);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBook, ModifyIncreaseRaisesTotal) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1, Side::Sell, 102, 50}), BookStatus::Ok);
    EXPECT_EQ(book.modify(1, 80), BookStatus::Ok);
    expectTop(book.bestAsk(), 102, 80);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBook, ModifiedOrderStaysUsable) {
    // after an increase (moved to the back with splice), the stored iterator must still work
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1, Side::Buy, 100, 50}), BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 2, Side::Buy, 100, 50}), BookStatus::Ok);
    EXPECT_EQ(book.modify(1, 80), BookStatus::Ok);
    expectTop(book.bestBid(), 100, 130);

    EXPECT_EQ(book.execute(1, 30), BookStatus::Ok);   // order 1 now holds 80
    expectTop(book.bestBid(), 100, 100);
    EXPECT_EQ(book.cancel(1), BookStatus::Ok);
    expectTop(book.bestBid(), 100, 50);                // only order 2 left
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBook, ModifyUnknownOrInvalidChangesNothing) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1, Side::Buy, 100, 50}), BookStatus::Ok);
    EXPECT_EQ(book.modify(42, 10), BookStatus::UnknownId);
    EXPECT_EQ(book.modify(1, 0), BookStatus::InvalidOrder);
    EXPECT_EQ(book.modify(1, -10), BookStatus::InvalidOrder);
    expectTop(book.bestBid(), 100, 50);
    EXPECT_TRUE(book.checkInvariants());
}


// Test Apply

TEST(OrderBook, ApplyRoutesEachType) {
    OrderBook book{};
    EXPECT_EQ(book.apply(OrderEvent{0, OrderType::Add, 1, Side::Buy, 100, 50}), BookStatus::Ok);
    expectTop(book.bestBid(), 100, 50);

    EXPECT_EQ(book.apply(OrderEvent{1, OrderType::Modify, 1, Side::None, 0, 30}), BookStatus::Ok);
    expectTop(book.bestBid(), 100, 30);

    EXPECT_EQ(book.apply(OrderEvent{2, OrderType::Execute, 1, Side::None, 0, 10}), BookStatus::Ok);
    expectTop(book.bestBid(), 100, 20);

    EXPECT_EQ(book.apply(OrderEvent{3, OrderType::Cancel, 1}), BookStatus::Ok);
    EXPECT_FALSE(book.bestBid().has_value());

    EXPECT_EQ(book.apply(OrderEvent{4, OrderType::None, 2, Side::Buy, 100, 10}), BookStatus::InvalidOrder);
    EXPECT_FALSE(book.bestBid().has_value());
    EXPECT_TRUE(book.checkInvariants());
}

// test uncross rule

TEST(OrderBook, BuyCrossingOneAskLevelRemovesIt) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1, Side::Sell, 100, 40}), BookStatus::Ok);  // stale ask
    EXPECT_EQ(book.add(OrderEvent{1, OrderType::Add, 2, Side::Buy, 101, 50}), BookStatus::Ok);   // crosses it
    EXPECT_FALSE(book.bestAsk().has_value());
    expectTop(book.bestBid(), 101, 50);          // the new order rests with its FULL quantity
    EXPECT_EQ(book.staleRemoved(), 1);
    EXPECT_EQ(book.cancel(1), BookStatus::UnknownId);   // the stale order is gone from the index too
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBook, BuyCrossingTwoAskLevelsRemovesBoth) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1, Side::Sell, 95, 110}), BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 2, Side::Sell, 95, 10}), BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 3, Side::Sell, 99, 50}), BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 4, Side::Sell, 102, 70}), BookStatus::Ok);  // not crossed
    EXPECT_EQ(book.add(OrderEvent{1, OrderType::Add, 5, Side::Buy, 100, 95}), BookStatus::Ok);
    expectTop(book.bestAsk(), 102, 70);          // 95 and 99 removed, 102 kept
    expectTop(book.bestBid(), 100, 95);
    EXPECT_EQ(book.staleRemoved(), 3);           // three orders: two at 95, one at 99
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBook, BuyEqualToAskCountsAsCrossing) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1, Side::Sell, 100, 40}), BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{1, OrderType::Add, 2, Side::Buy, 100, 50}), BookStatus::Ok);
    EXPECT_FALSE(book.bestAsk().has_value());
    EXPECT_EQ(book.staleRemoved(), 1);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBook, NonCrossingAddRemovesNothing) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1, Side::Sell, 102, 40}), BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{1, OrderType::Add, 2, Side::Buy, 101, 50}), BookStatus::Ok);
    expectTop(book.bestAsk(), 102, 40);
    expectTop(book.bestBid(), 101, 50);
    EXPECT_EQ(book.staleRemoved(), 0);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBook, SellCrossingBidLevelsRemovesThem) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1, Side::Buy, 102, 30}), BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 2, Side::Buy, 101, 20}), BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 3, Side::Buy, 99, 60}), BookStatus::Ok);    // not crossed
    EXPECT_EQ(book.add(OrderEvent{1, OrderType::Add, 4, Side::Sell, 100, 80}), BookStatus::Ok);
    expectTop(book.bestBid(), 99, 60);           // 102 and 101 removed, 99 kept
    expectTop(book.bestAsk(), 100, 80);
    EXPECT_EQ(book.staleRemoved(), 2);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBook, NoneSideRejectedBeforeAnyChange) {
    OrderBook book{};
    EXPECT_EQ(book.add(OrderEvent{0, OrderType::Add, 1, Side::Sell, 100, 40}), BookStatus::Ok);
    EXPECT_EQ(book.add(OrderEvent{1, OrderType::Add, 2, Side::None, 101, 50}), BookStatus::InvalidOrder);
    expectTop(book.bestAsk(), 100, 40);          // nothing removed
    EXPECT_EQ(book.staleRemoved(), 0);
    EXPECT_TRUE(book.checkInvariants());
}

// OrderBOokqueue
TEST(OrderBookQueueAhead, UnknownID) {
    OrderBook book{};
    book.add(OrderEvent{0, OrderType::Add, 1, Side::Sell, 100, 40});
    EXPECT_EQ(book.queueAhead(3), std::nullopt);
    EXPECT_TRUE(book.checkInvariants());
}


TEST(OrderBookQueueAhead, SingleOrder) {
    OrderBook book{};
    book.add(OrderEvent{0, OrderType::Add, 1, Side::Sell, 100, 40});
    EXPECT_EQ(book.queueAhead(1), 0);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBookQueueAhead, MultipleSameLevelOrders) {
    OrderBook book{};
    book.add(OrderEvent{0, OrderType::Add, 1, Side::Buy, 100, 30});
    book.add(OrderEvent{0, OrderType::Add, 2, Side::Buy, 100, 20});
    book.add(OrderEvent{0, OrderType::Add, 3, Side::Buy, 100, 60});
    EXPECT_EQ(book.queueAhead(3), 50);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBookQueueAhead, OtherOrdersIrrelevant) {
    OrderBook book{};
    // three bids at 100 (the queue we test)
    book.add(OrderEvent{0, OrderType::Add, 1, Side::Buy, 100, 30});
    book.add(OrderEvent{0, OrderType::Add, 2, Side::Buy, 100, 20});
    book.add(OrderEvent{0, OrderType::Add, 3, Side::Buy, 100, 60});
    // noise: better bid, worse bid, asks above the best bid (no crossing)
    book.add(OrderEvent{0, OrderType::Add, 4, Side::Buy, 101, 70});
    book.add(OrderEvent{0, OrderType::Add, 5, Side::Buy, 99, 500});
    book.add(OrderEvent{0, OrderType::Add, 6, Side::Sell, 102, 1000});
    book.add(OrderEvent{0, OrderType::Add, 7, Side::Sell, 102, 40});

    EXPECT_EQ(book.queueAhead(1), 0);
    EXPECT_EQ(book.queueAhead(2), 30);
    EXPECT_EQ(book.queueAhead(3), 50);   // only #1 and #2 count
    EXPECT_EQ(book.queueAhead(4), 0);    // alone at 101
    EXPECT_EQ(book.queueAhead(5), 0);    // alone at 99
    EXPECT_EQ(book.queueAhead(7), 1000); // ask side, behind #6
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBookQueueAhead, ExecuteCancelThenEvaluate) {
    OrderBook book{};
    book.add(OrderEvent{0, OrderType::Add, 1, Side::Buy, 100, 30});
    book.add(OrderEvent{0, OrderType::Add, 2, Side::Buy, 100, 20});
    EXPECT_EQ(book.queueAhead(2), 30);

    // partial execute ahead: #1 stays, value ahead of #2 goes down
    EXPECT_EQ(book.execute(1, 10), BookStatus::Ok);
    EXPECT_EQ(book.queueAhead(1), 0);
    EXPECT_EQ(book.queueAhead(2), 20);

    // full execute ahead: #1 removed
    EXPECT_EQ(book.execute(1, 20), BookStatus::Ok);
    EXPECT_EQ(book.queueAhead(1), std::nullopt);
    EXPECT_EQ(book.queueAhead(2), 0);

    // cancel ahead
    book.add(OrderEvent{0, OrderType::Add, 3, Side::Buy, 100, 60});
    EXPECT_EQ(book.queueAhead(3), 20);
    EXPECT_EQ(book.cancel(2), BookStatus::Ok);
    EXPECT_EQ(book.queueAhead(3), 0);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(OrderBookQueueAhead, Modify) {
    OrderBook book{};
    book.add(OrderEvent{0, OrderType::Add, 1, Side::Buy, 100, 30});
    book.modify(1,15);
    EXPECT_EQ(book.queueAhead(1), 0);
    book.add(OrderEvent{0, OrderType::Add, 2, Side::Buy, 100, 100});
    book.modify(1,200);
    EXPECT_EQ(book.queueAhead(1), 100);
    EXPECT_TRUE(book.checkInvariants());
}







