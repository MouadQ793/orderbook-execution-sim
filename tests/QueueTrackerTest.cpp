#include <gtest/gtest.h>
#include <optional>
#include "ob/OrderBook.h"
#include "ob/QueueTracker.h"

namespace {
    OrderEvent addEvent(std::int64_t id, Side side, std::int64_t price, std::int64_t qty) {
        return OrderEvent{0, OrderType::Add, id, side, price, qty};
    }
    OrderEvent cancelEvent(std::int64_t id) {
        return OrderEvent{0, OrderType::Cancel, id, Side::None, 0, 0};
    }
    OrderEvent executeEvent(std::int64_t id, std::int64_t qty) {
        return OrderEvent{0, OrderType::Execute, id, Side::None, 0, qty};
    }
    OrderEvent modifyEvent(std::int64_t id, std::int64_t qty) {
        return OrderEvent{0, OrderType::Modify, id, Side::None, 0, qty};
    }
    // same order as in the replay: tracker before, book, tracker after
    void step(OrderBook& book, QueueTracker& tracker, const OrderEvent& event) {
        tracker.updateBeforeApply(event, book);
        EXPECT_EQ(book.apply(event), BookStatus::Ok);
        tracker.updateAfterApply(event, book);
    }
    // the tracker must always agree with the exact walk
    void expectAhead(const OrderBook& book, const QueueTracker& tracker, std::int64_t id, std::int64_t expected) {
        EXPECT_EQ(tracker.ahead(id), std::optional<std::int64_t>{expected});
        EXPECT_EQ(book.queueAhead(id), std::optional<std::int64_t>{expected});
    }
}

TEST(QueueTracker, StartTracking) {
    OrderBook book{};
    QueueTracker tracker{};
    step(book, tracker, addEvent(1, Side::Buy, 100, 30));
    step(book, tracker, addEvent(2, Side::Buy, 100, 20));
    EXPECT_FALSE(tracker.startTracking(9, book));
    EXPECT_TRUE(tracker.startTracking(2, book));
    EXPECT_FALSE(tracker.startTracking(2, book));
    expectAhead(book, tracker, 2, 30);
    EXPECT_EQ(tracker.ahead(1), std::nullopt);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(QueueTracker, OrderAheadExecuteCancel) {
    OrderBook book{};
    QueueTracker tracker{};
    step(book, tracker, addEvent(1, Side::Buy, 100, 30));
    step(book, tracker, addEvent(2, Side::Buy, 100, 20));
    step(book, tracker, addEvent(3, Side::Buy, 100, 60));
    ASSERT_TRUE(tracker.startTracking(3, book));
    expectAhead(book, tracker, 3, 50);

    step(book, tracker, executeEvent(1, 10));
    expectAhead(book, tracker, 3, 40);
    step(book, tracker, cancelEvent(2));
    expectAhead(book, tracker, 3, 20);
    step(book, tracker, executeEvent(1, 20));
    expectAhead(book, tracker, 3, 0);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(QueueTracker, OrderAheadModify) {
    OrderBook book{};
    QueueTracker tracker{};
    step(book, tracker, addEvent(1, Side::Buy, 100, 100));
    step(book, tracker, addEvent(2, Side::Buy, 100, 40));
    ASSERT_TRUE(tracker.startTracking(2, book));
    expectAhead(book, tracker, 2, 100);

    step(book, tracker, modifyEvent(1, 30));
    expectAhead(book, tracker, 2, 30);
    step(book, tracker, modifyEvent(1, 500));
    expectAhead(book, tracker, 2, 0);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(QueueTracker, OrdersBehindOrElsewhereIgnored) {
    OrderBook book{};
    QueueTracker tracker{};
    step(book, tracker, addEvent(1, Side::Buy, 100, 40));
    step(book, tracker, addEvent(2, Side::Buy, 100, 25));
    ASSERT_TRUE(tracker.startTracking(2, book));
    step(book, tracker, addEvent(3, Side::Buy, 100, 70));
    step(book, tracker, addEvent(4, Side::Buy, 99, 80));
    step(book, tracker, addEvent(5, Side::Sell, 101, 90));
    expectAhead(book, tracker, 2, 40);

    step(book, tracker, executeEvent(3, 10));
    step(book, tracker, modifyEvent(3, 500));
    step(book, tracker, cancelEvent(3));
    step(book, tracker, executeEvent(4, 30));
    step(book, tracker, cancelEvent(4));
    step(book, tracker, modifyEvent(5, 10));
    step(book, tracker, cancelEvent(5));
    expectAhead(book, tracker, 2, 40);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(QueueTracker, TrackedOrderOwnMessages) {
    OrderBook book{};
    QueueTracker tracker{};
    step(book, tracker, addEvent(1, Side::Sell, 105, 50));
    step(book, tracker, addEvent(2, Side::Sell, 105, 30));
    step(book, tracker, addEvent(3, Side::Sell, 105, 20));
    ASSERT_TRUE(tracker.startTracking(2, book));
    expectAhead(book, tracker, 2, 50);

    step(book, tracker, executeEvent(2, 10));
    expectAhead(book, tracker, 2, 50);
    step(book, tracker, modifyEvent(2, 5));
    expectAhead(book, tracker, 2, 50);
    step(book, tracker, modifyEvent(2, 100));
    expectAhead(book, tracker, 2, 70);
    step(book, tracker, cancelEvent(2));
    EXPECT_EQ(tracker.ahead(2), std::nullopt);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(QueueTracker, TrackedOrderFullyExecuted) {
    OrderBook book{};
    QueueTracker tracker{};
    step(book, tracker, addEvent(1, Side::Buy, 100, 30));
    ASSERT_TRUE(tracker.startTracking(1, book));
    step(book, tracker, executeEvent(1, 30));
    EXPECT_EQ(tracker.ahead(1), std::nullopt);
    EXPECT_TRUE(book.checkInvariants());
}

TEST(QueueTracker, UncrossRemovesTrackedOrder) {
    OrderBook book{};
    QueueTracker tracker{};
    step(book, tracker, addEvent(1, Side::Buy, 100, 30));
    step(book, tracker, addEvent(2, Side::Buy, 99, 30));
    ASSERT_TRUE(tracker.startTracking(1, book));
    ASSERT_TRUE(tracker.startTracking(2, book));
    step(book, tracker, addEvent(3, Side::Sell, 100, 50));
    EXPECT_EQ(tracker.ahead(1), std::nullopt);
    expectAhead(book, tracker, 2, 0);
    EXPECT_TRUE(book.checkInvariants());
}