
#include <gtest/gtest.h>
#include "ob/parsing.h"



TEST(FitOrderEvent, ParsesValidAdd) {
    // to check if parsing a valid addition order works as intended, giving a valid input, using the function and checking if the output is valid
    std::array<std::string_view, fieldCount> fields{splitOrder("0,A,1,B,100,50")};
    std::optional<OrderEvent> event{fitOrderEvent(fields)};

    // Checking

    ASSERT_TRUE(event.has_value());
    EXPECT_EQ(event->timestamp, 0);
    EXPECT_EQ(event->type, OrderType::Add);
    EXPECT_EQ(event->orderId, 1);
    EXPECT_EQ(event->side, Side::Buy);
    EXPECT_EQ(event->price, 100);
    EXPECT_EQ(event->quantity, 50);
}

TEST(FitOrderEvent, ParsesValidCancel) {
    // Same, checking cancel order
    std::array<std::string_view, fieldCount> fields{splitOrder("3000,C,3,,,")};
    std::optional<OrderEvent> event{fitOrderEvent(fields)};

    // Checking

    ASSERT_TRUE(event.has_value());
    EXPECT_EQ(event->timestamp, 3000);
    EXPECT_EQ(event->type, OrderType::Cancel);
    EXPECT_EQ(event->orderId, 3);
    EXPECT_EQ(event->side, Side::None);
    EXPECT_EQ(event->price, 0);
    EXPECT_EQ(event->quantity, 0);
}

TEST(FitOrderEvent, ParsesValidExecute) {
    // Same, checking execute order
    std::array<std::string_view, fieldCount> fields{splitOrder("2350,E,5,,,150")};
    std::optional<OrderEvent> event{fitOrderEvent(fields)};

    // Checking

    ASSERT_TRUE(event.has_value());
    EXPECT_EQ(event->timestamp, 2350);
    EXPECT_EQ(event->type, OrderType::Execute);
    EXPECT_EQ(event->orderId, 5);
    EXPECT_EQ(event->side, Side::None);
    EXPECT_EQ(event->price, 0);
    EXPECT_EQ(event->quantity, 150);
}

TEST(FitOrderEvent, ParsesValidModify) {
    // Same, checking Modify order
    std::array<std::string_view, fieldCount> fields{splitOrder("4150,M,9,,,120")};
    std::optional<OrderEvent> event{fitOrderEvent(fields)};

    // Checking

    ASSERT_TRUE(event.has_value());
    EXPECT_EQ(event->timestamp, 4150);
    EXPECT_EQ(event->type, OrderType::Modify);
    EXPECT_EQ(event->orderId, 9);
    EXPECT_EQ(event->side, Side::None);
    EXPECT_EQ(event->price, 0);
    EXPECT_EQ(event->quantity, 120);
}

TEST(FitOrderEvent, UnkownType) {
    // Same, checking not known type order
    std::array<std::string_view, fieldCount> fields{splitOrder("5045,Z,6,,,")};
    std::optional<OrderEvent> event{fitOrderEvent(fields)};

    // Checking
    ASSERT_FALSE(event.has_value());
}

TEST(FitOrderEvent, PriceNotNumber) {
    // Same, checking when price not a number
    std::array<std::string_view, fieldCount> fields{splitOrder("5045,A,6,S,abc,200")};
    std::optional<OrderEvent> event{fitOrderEvent(fields)};
    // Checking
    ASSERT_FALSE(event.has_value());
}

TEST(FitOrderEvent, AddNoSide) {
    // Same, checking with there is no side to the order
    std::array<std::string_view, fieldCount> fields{splitOrder("5045,A,6,,105,200")};
    std::optional<OrderEvent> event{fitOrderEvent(fields)};
    // Checking
    ASSERT_FALSE(event.has_value());
}

