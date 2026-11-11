//
// Created by mouad-sehli on 11/11/26.
//


#include "gtest/gtest.h"
#include "ob/Engine.h"
#include "ob/OrderBook.h"

TEST(CorrectOrder, TimeStampsOrdering) {

   OrderEvent orderA {25,OrderType::Add,5,Side::Buy,100,1490};
    OrderEvent orderB {20,OrderType::Add,5,Side::Buy,100,1500};
    ScheduledEvent scheduledEventA {25,Priority::Market,5,orderA};
    ScheduledEvent scheduledEventB {20,Priority::Market,8,orderB};

    EXPECT_TRUE(scheduledEventA > scheduledEventB);
    EXPECT_FALSE(scheduledEventA < scheduledEventB);
}

TEST(CorrectOrder, TypeOrdering) {
    ScheduledEvent scheduledEventA;
    ScheduledEvent scheduledEventB;
    OrderEvent orderA{};
    OrderEvent orderB{};
    orderA = {25,OrderType::Add,5,Side::Buy,100,1490};
    orderB = {25,OrderType::Add,5,Side::Buy,100,1500};
    scheduledEventA = {25,Priority::Market,8,orderA};
    scheduledEventB = {25,Priority::Strategy,5,orderB};

    EXPECT_TRUE(scheduledEventA < scheduledEventB);
    EXPECT_FALSE(scheduledEventA > scheduledEventB);
}

TEST(CorrectOrder, TicketOrdering) {

    OrderEvent orderA  {25,OrderType::Add,5,Side::Buy,100,1490};
    OrderEvent orderB  {25,OrderType::Add,5,Side::Buy,100,1500};
    ScheduledEvent scheduledEventA  {25,Priority::Market,5,orderA};
    ScheduledEvent scheduledEventB  {25,Priority::Market,8,orderB};

    EXPECT_TRUE(scheduledEventA < scheduledEventB);
    EXPECT_FALSE(scheduledEventA > scheduledEventB);
}

TEST(CorrectOrder, QueueTest) {

    OrderEvent orderA  {35,OrderType::Add,5,Side::Buy,100,1490};
    OrderEvent orderB  {15,OrderType::Add,5,Side::Buy,100,1500};
    OrderEvent orderC  {25,OrderType::Add,5,Side::Buy,100,1490};
    OrderEvent orderD  {25,OrderType::Add,5,Side::Buy,100,1500};
    ScheduledEvent scheduledEventA  {35,Priority::Market,5,orderA};
    ScheduledEvent scheduledEventB  {15,Priority::Market,8,orderB};
    ScheduledEvent scheduledEventC  {25,Priority::Strategy,10,orderC};
    ScheduledEvent scheduledEventD  {25,Priority::Market,12,orderD};

    EventQueue eventQueue{};
    eventQueue.push(scheduledEventA);
    eventQueue.push(scheduledEventB);
    eventQueue.push(scheduledEventC);
    eventQueue.push(scheduledEventD);

    EXPECT_EQ(eventQueue.top(), scheduledEventB);
    eventQueue.pop();
    EXPECT_EQ(eventQueue.top(), scheduledEventD);
    eventQueue.pop();
    EXPECT_EQ(eventQueue.top(), scheduledEventC);
    eventQueue.pop();
    EXPECT_EQ(eventQueue.top(), scheduledEventA);
    eventQueue.pop();
    EXPECT_TRUE(eventQueue.empty());
}

