
#include <gtest/gtest.h>
#include "ob/parsing.h"


// Fitting order events
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

// Checking time converters

TEST(timeToNano, paddingRequired) {
    std::string_view lobsterTime{"34200.01399412"};
    const std::optional<std::int64_t> time{timeToNano(lobsterTime)};

    ASSERT_TRUE(time.has_value());
    EXPECT_EQ(*time, 13994120);
}

TEST(timeToNano, NineDigits) {
    std::string_view lobsterTime{"34200.015247805"};
    const std::optional<std::int64_t> time{timeToNano(lobsterTime)};

    ASSERT_TRUE(time.has_value());
    EXPECT_EQ(*time, 15247805);
}

TEST(timeToNano, OneDigitOneSecondAfterOpen) {
    std::string_view lobsterTime{"34201.5"};
    const std::optional<std::int64_t> time{timeToNano(lobsterTime)};

    ASSERT_TRUE(time.has_value());
    EXPECT_EQ(*time, 1'500'000'000);
}

TEST(timeToNano, NoDot) {
    std::string_view lobsterTime{"34200"};
    const std::optional<std::int64_t> time{timeToNano(lobsterTime)};

    ASSERT_TRUE(time.has_value());
    EXPECT_EQ(*time, 0);
}

TEST(timeToNano, RejectsNonNumericFraction) {
    std::string_view lobsterTime{"34200.abc"};
    const std::optional<std::int64_t> time{timeToNano(lobsterTime)};

    ASSERT_FALSE(time.has_value());

}

TEST(timeToNano, MoreThanNineDigits) {
    std::string_view lobsterTime{"34200.1234567891"};
    const std::optional<std::int64_t> time{timeToNano(lobsterTime)};

    ASSERT_TRUE(time.has_value());
    EXPECT_EQ(time, 123'456'789);
}
// Checking Lobster Parser

TEST(parseLobsterEvent, ParsesNewOrder) {
    const std::array<std::string_view, fieldCount> fields{splitOrder("34200.01399412,1,16116348,100,310500,-1")};
    std::optional<LobsterEvent> event{parseLobsterEvent(fields)};

    ASSERT_TRUE(event.has_value());
    EXPECT_EQ(event->timestamp, 13'994'120);
    EXPECT_EQ(event->type, LobsterType::NewOrder);
    EXPECT_EQ(event->orderId, 16116348);
    EXPECT_EQ(event->side, Side::Sell);
    EXPECT_EQ(event->price, 3105);
    EXPECT_EQ(event->quantity, 100);
}

TEST(parseLobsterEvent, RejectsUnkownType) {
    const std::array<std::string_view, fieldCount> fields{splitOrder("34200.0,8,1,100,310500,1")};
    std::optional<LobsterEvent> event{parseLobsterEvent(fields)};
    ASSERT_FALSE(event.has_value());
}

TEST(parseLobsterEvent, RejectsSubTickPrice	) {
    const std::array<std::string_view, fieldCount> fields{splitOrder("34200.0,1,1,100,310550,1")};
    std::optional<LobsterEvent> event{parseLobsterEvent(fields)};
    ASSERT_FALSE(event.has_value());
}




















