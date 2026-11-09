//
// Created by mouad-sehli on 11/9/26.
//


#include "ob/Indicators.h"
#include <sstream>
#include <iostream>
#include <cstdint>
#include <cmath>
#include "ob/utils.h"

void Indicators::update(const OrderBook& orderBook, const OrderEvent&) {
    // check book to see if there was a change in the prices
    // if there was check there is both a bid and ask to compute this->
    // update previous ones

    const std::optional<TopOfBook>  currentTopAsk{orderBook.bestAsk()};
    const std::optional<TopOfBook>  currentTopBid{orderBook.bestBid()};
    std::int64_t currentBestAsk{};
    std::int64_t currentBestBid{};

    if (currentTopAsk) currentBestAsk = currentTopAsk->price;
    if (currentTopBid) currentBestBid = currentTopBid->price;

    // compare if mid changed or not, if previous one exists and if Bid Ask/top still exist
    bool newMid {(currentBestAsk != m_prevAskPrice) || (currentBestBid != m_prevBidPrice)};
    this->m_midChanged = (newMid && this->m_exists && currentTopAsk && currentTopBid);


    if (currentTopAsk && currentTopBid)  {
        this->m_spread = currentBestAsk - currentBestBid;
        this->m_doubleMid = currentBestBid + currentBestAsk;
        this->m_exists = true;
        this->m_imbalance1 = imbalance(currentTopBid->totalQty, currentTopAsk->totalQty);
        this->m_imbalancek = imbalance(orderBook.depth(Side::Buy,imbalanceDepth), orderBook.depth(Side::Sell,imbalanceDepth));
    }
    else {this->m_exists = false;}
    if (this->m_midChanged) {
        // return in ticks: the double mid is in half-ticks
        const double r{static_cast<double>(this->m_doubleMid - this->m_prevDoubleMid) / 2.0};
        const double d{r - this->m_mean};
        this->m_mean = (1.0 - this->m_alpha) * this->m_mean + this->m_alpha * r;
        this->m_variance = (1.0 - this->m_alpha) * (this->m_variance + this->m_alpha * d * d);
    }
    // previous mid = the mid after THIS message, kept for the next one
    if (this->m_exists) this->m_prevDoubleMid = this->m_doubleMid;


    // store the current prices as the old ones
    this->m_prevAskPrice = currentBestAsk;
    this->m_prevBidPrice = currentBestBid;

}


std::string Indicators::toCsvLine(std::int64_t ts) const {

    std::stringstream os{};
    if (m_exists) {
        os << ts << ',' << this->m_spread << ',' << this->m_doubleMid << ',' << this->m_midChanged << ',' << this->m_imbalance1 << ',' << this->m_imbalancek << ',' <<
            this->m_mean << ',' << std::sqrt(this->m_variance) << '\n';
    } else {
        os << ts << ",,," << this->m_midChanged << ",,";
    }
    return os.str();
}

double Indicators::imbalance(std::int64_t bidQty, std::int64_t askQty) {
    if (bidQty + askQty == 0) {std::cerr << "imbalance: zero total quantity\n"; return 0.0;}
    double denom {static_cast<double>(bidQty + askQty)};
    double num {static_cast<double>(bidQty - askQty)};
    return num/denom;
}


