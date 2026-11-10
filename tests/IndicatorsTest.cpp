#include <gtest/gtest.h>
#include <cstdint>
#include "ob/OrderBook.h"
#include "ob/Indicators.h"

// Every expected value below is computed by hand in the comments.
// tau = 1 s = 1e9 ns. With dt = tau * ln 2, alpha = 1 - exp(-ln 2) = 0.5 exactly
// (up to the rounding of dt to a whole nanosecond, hence EXPECT_NEAR).

namespace {
    constexpr std::int64_t halfLife{693'147'181};   // tau * ln 2 in ns -> alpha = 0.5
    constexpr double tol{1e-6};

    struct Replay {
        OrderBook book{};
        Indicators indicators{};
        // same order as in replay: book first, then indicators
        void step(const OrderEvent& event) {
            EXPECT_EQ(book.apply(event), BookStatus::Ok);
            indicators.update(book, event);
        }
        void add(std::int64_t ts, std::int64_t id, Side side, std::int64_t price, std::int64_t qty) {
            step(OrderEvent{ts, OrderType::Add, id, side, price, qty});
        }
        void cancel(std::int64_t ts, std::int64_t id) {
            step(OrderEvent{ts, OrderType::Cancel, id, Side::None, 0, 0});
        }
    };
}

// ---------- spread, mid, mid change, imbalance ----------

TEST(Indicators, SpreadMidAndMidChange) {
    Replay r{};
    r.add(0, 1, Side::Buy, 100, 10);
    EXPECT_FALSE(r.indicators.getexists());          // no ask yet
    EXPECT_FALSE(r.indicators.getmidchanged());

    r.add(0, 2, Side::Sell, 102, 10);
    EXPECT_TRUE(r.indicators.getexists());
    EXPECT_EQ(r.indicators.getSpread(), 2);
    EXPECT_EQ(r.indicators.getdoublemid(), 202);
    EXPECT_FALSE(r.indicators.getmidchanged());      // no mid before this message

    r.add(0, 3, Side::Buy, 101, 10);                 // better bid: mid moves up half a tick
    EXPECT_EQ(r.indicators.getSpread(), 1);
    EXPECT_EQ(r.indicators.getdoublemid(), 203);
    EXPECT_TRUE(r.indicators.getmidchanged());

    r.add(0, 4, Side::Buy, 99, 10);                  // deeper bid: nothing at the top changes
    EXPECT_EQ(r.indicators.getdoublemid(), 203);
    EXPECT_FALSE(r.indicators.getmidchanged());
}

TEST(Indicators, Imbalance) {
    Replay r{};
    r.add(0, 1, Side::Buy, 100, 10);
    r.add(0, 2, Side::Sell, 101, 30);
    // level 1: (10 - 30) / (10 + 30) = -0.5
    EXPECT_NEAR(r.indicators.getImbalance1(), -0.5, tol);
    r.add(0, 3, Side::Buy, 99, 60);
    // level 1 unchanged; 5 levels: bid 10 + 60 = 70, ask 30 -> (70 - 30) / 100 = 0.4
    EXPECT_NEAR(r.indicators.getImbalance1(), -0.5, tol);
    EXPECT_NEAR(r.indicators.getImbalanceK(), 0.4, tol);
}

// ---------- EWMA of mid returns (alpha = 0.05 per mid change) ----------

TEST(Indicators, ReturnEwmaByHand) {
    Replay r{};
    r.add(0, 1, Side::Buy, 100, 10);
    r.add(0, 2, Side::Sell, 102, 10);               // double mid 202
    EXPECT_NEAR(r.indicators.getRetMean(), 0.0, tol);
    EXPECT_NEAR(r.indicators.getRetVariance(), 0.0, tol);

    // move 1: double mid 202 -> 203, r = +0.5 tick
    // d = 0.5 - 0 = 0.5
    // mean = 0.05 * 0.5 = 0.025
    // var  = 0.95 * (0 + 0.05 * 0.25) = 0.011875
    r.add(0, 3, Side::Buy, 101, 10);
    EXPECT_NEAR(r.indicators.getRetMean(), 0.025, 1e-12);
    EXPECT_NEAR(r.indicators.getRetVariance(), 0.011875, 1e-12);

    // move 2: double mid 203 -> 202, r = -0.5 tick
    // d = -0.5 - 0.025 = -0.525
    // mean = 0.95 * 0.025 + 0.05 * (-0.5) = -0.00125
    // var  = 0.95 * (0.011875 + 0.05 * 0.275625) = 0.0243734375
    r.cancel(0, 3);
    EXPECT_NEAR(r.indicators.getRetMean(), -0.00125, 1e-12);
    EXPECT_NEAR(r.indicators.getRetVariance(), 0.0243734375, 1e-12);
}

TEST(Indicators, NoMidChangeKeepsReturnEwma) {
    Replay r{};
    r.add(0, 1, Side::Buy, 100, 10);
    r.add(0, 2, Side::Sell, 102, 10);
    r.add(0, 3, Side::Buy, 101, 10);                 // one mid change
    const double mean{r.indicators.getRetMean()};
    const double variance{r.indicators.getRetVariance()};

    // messages that leave the mid where it is: no zero return must be added
    r.add(0, 4, Side::Buy, 99, 10);
    r.add(0, 5, Side::Sell, 103, 10);
    r.add(0, 6, Side::Buy, 101, 50);                 // joins the best bid queue
    r.cancel(0, 4);
    EXPECT_FALSE(r.indicators.getmidchanged());
    EXPECT_NEAR(r.indicators.getRetMean(), mean, 1e-12);
    EXPECT_NEAR(r.indicators.getRetVariance(), variance, 1e-12);
}

// ---------- time-weighted spread and depth (alpha = 1 - exp(-dt / tau)) ----------

TEST(Indicators, FirstSpreadIsNotWeighted) {
    Replay r{};
    r.add(0, 1, Side::Buy, 100, 10);
    r.add(5'000'000'000, 2, Side::Sell, 101, 10);    // spread appears after 5 s with no spread before
    EXPECT_NEAR(r.indicators.getSpreadMean(), 0.0, tol);
    EXPECT_NEAR(r.indicators.getDepthMean(), 0.0, tol);
}

TEST(Indicators, SpreadMeanUsesSpreadInForce) {
    Replay r{};
    r.add(0, 1, Side::Buy, 100, 10);
    r.add(0, 2, Side::Sell, 101, 10);                // spread 1 from t = 0
    EXPECT_NEAR(r.indicators.getSpreadMean(), 0.0, tol);

    // spread 1 was in force for halfLife: mean = 0.5 * 0 + 0.5 * 1 = 0.5
    r.add(halfLife, 3, Side::Sell, 102, 10);
    EXPECT_NEAR(r.indicators.getSpreadMean(), 0.5, tol);

    // still spread 1 during the gap (the new spread 2 has lasted 0 s): 0.5 * 0.5 + 0.5 * 1 = 0.75
    r.cancel(2 * halfLife, 2);
    EXPECT_EQ(r.indicators.getSpread(), 2);
    EXPECT_NEAR(r.indicators.getSpreadMean(), 0.75, tol);

    // same timestamp: the 2-tick spread lasted 0 s, so alpha = 0 and nothing changes
    r.add(2 * halfLife, 4, Side::Sell, 101, 10);
    EXPECT_EQ(r.indicators.getSpread(), 1);
    EXPECT_NEAR(r.indicators.getSpreadMean(), 0.75, tol);

    // spread 1 in force again for halfLife: 0.5 * 0.75 + 0.5 * 1 = 0.875
    r.add(3 * halfLife, 5, Side::Buy, 99, 10);
    EXPECT_NEAR(r.indicators.getSpreadMean(), 0.875, tol);
}

TEST(Indicators, DepthMeanUsesDepthInForce) {
    Replay r{};
    r.add(0, 1, Side::Buy, 100, 10);
    r.add(0, 2, Side::Sell, 101, 30);                // depth at best = 10 + 30 = 40

    // depth 40 in force for halfLife: 0.5 * 0 + 0.5 * 40 = 20, then depth becomes 60
    r.add(halfLife, 3, Side::Buy, 100, 20);
    EXPECT_NEAR(r.indicators.getDepthMean(), 20.0, tol);

    // depth 60 in force for halfLife: 0.5 * 20 + 0.5 * 60 = 40
    r.add(2 * halfLife, 4, Side::Buy, 99, 500);      // deeper order: depth at best stays 60
    EXPECT_NEAR(r.indicators.getDepthMean(), 40.0, tol);
}

// ---------- message intensity (jump 1 / tau per message, in messages per second) ----------

TEST(Indicators, IntensityByHand) {
    Replay r{};
    r.add(0, 1, Side::Buy, 100, 10);                 // 0 * ... + 1 = 1 (counted even with no spread)
    EXPECT_NEAR(r.indicators.getIntensity(), 1.0, tol);
    r.add(0, 2, Side::Sell, 101, 10);                // same timestamp: 1 + 1 = 2
    EXPECT_NEAR(r.indicators.getIntensity(), 2.0, tol);
    r.add(halfLife, 3, Side::Buy, 99, 10);           // 0.5 * 2 + 1 = 2
    EXPECT_NEAR(r.indicators.getIntensity(), 2.0, tol);
}

TEST(Indicators, IntensitySettlesAtMessageRate) {
    // one message every 10 ms = 100 messages per second, for 20 s
    // steady state: 1 / (1 - exp(-0.01)) = 100.50 (close to 100, as derived)
    Replay r{};
    r.add(0, 1, Side::Buy, 100, 10);
    r.add(0, 2, Side::Sell, 200, 10);
    std::int64_t ts{0};
    for (std::int64_t id{3}; id < 2003; ++id) {
        ts += 10'000'000;
        r.add(ts, id, Side::Buy, 50 + id % 40, 1);   // deep bids, the top never changes
    }
    EXPECT_NEAR(r.indicators.getIntensity(), 100.5, 0.1);
}

// ---------- order flow imbalance (Cont, Kukanov & Stoikov 2014), level 1 ----------
// e = 1{Pb >= Pb_prev} qb - 1{Pb <= Pb_prev} qb_prev - 1{Pa <= Pa_prev} qa + 1{Pa >= Pa_prev} qa_prev

namespace {
    // bid 100 x 50, ask 101 x 40 (and a second ask level 102 x 30)
    void startBook(Replay& r) {
        r.add(0, 1, Side::Buy, 100, 50);
        r.add(0, 2, Side::Sell, 101, 40);
        r.add(0, 3, Side::Sell, 102, 30);
    }
}

TEST(IndicatorsOfi, ZeroWithoutBothSides) {
    Replay r{};
    r.add(0, 1, Side::Buy, 100, 50);                 // no ask: no OFI
    EXPECT_EQ(r.indicators.getOfi(), 0);
    r.add(0, 2, Side::Sell, 101, 40);                // both sides now, but not before: no OFI
    EXPECT_EQ(r.indicators.getOfi(), 0);
}

TEST(IndicatorsOfi, BidQuantityAddedAtSamePrice) {
    Replay r{};
    startBook(r);
    // bid price unchanged: 50 -> 70, so e = 70 - 50 = +20 (ask unchanged: 40 - 40 = 0)
    r.add(0, 4, Side::Buy, 100, 20);
    EXPECT_EQ(r.indicators.getOfi(), 20);
}

TEST(IndicatorsOfi, AskQuantityAddedAtSamePrice) {
    Replay r{};
    startBook(r);
    // ask price unchanged: 40 -> 50, so e = -(50) + 40 = -10 (selling pressure)
    r.add(0, 4, Side::Sell, 101, 10);
    EXPECT_EQ(r.indicators.getOfi(), -10);
}

TEST(IndicatorsOfi, BestAskEatenPriceGoesUp) {
    Replay r{};
    startBook(r);
    // ask 101 x 40 fully executed, best ask becomes 102 x 30
    // ask price went up: only + qa_prev counts, e = +40 (buying pressure)
    r.step(OrderEvent{0, OrderType::Execute, 2, Side::None, 0, 40});
    EXPECT_EQ(r.indicators.getOfi(), 40);
}

TEST(IndicatorsOfi, BestBidLevelEmptiedPriceGoesDown) {
    Replay r{};
    startBook(r);
    r.add(0, 4, Side::Buy, 99, 70);
    // bid 100 x 50 cancelled, best bid becomes 99 x 70
    // bid price went down: only - qb_prev counts, e = -50
    r.cancel(0, 1);
    EXPECT_EQ(r.indicators.getOfi(), -50);
}

TEST(IndicatorsOfi, NewBetterBidCountsWholeLevel) {
    Replay r{};
    startBook(r);
    // ask 101 cancelled first so a bid at 101 does not cross: best ask becomes 102 x 30 (e = +40)
    r.cancel(0, 2);
    // new best bid 101 x 25: bid price went up, only + qb counts, e = +25
    r.add(0, 4, Side::Buy, 101, 25);
    EXPECT_EQ(r.indicators.getOfi(), 25);
}

TEST(IndicatorsOfi, DeepMessageAndResetGiveZero) {
    Replay r{};
    startBook(r);
    r.add(0, 4, Side::Buy, 100, 20);                 // +20
    EXPECT_EQ(r.indicators.getOfi(), 20);
    r.add(0, 5, Side::Buy, 95, 500);                 // deeper level: top unchanged, e = 0
    EXPECT_EQ(r.indicators.getOfi(), 0);
    // empty the bid side: cancel 100 x 50, 100 x 20, then 95 x 500
    r.cancel(0, 1);
    r.cancel(0, 4);
    EXPECT_EQ(r.indicators.getOfi(), -20);           // best bid 100 -> 95: e = -(20)
    r.cancel(0, 5);                                  // no bid left: e = 0, not the previous -20
    EXPECT_EQ(r.indicators.getOfi(), 0);
}