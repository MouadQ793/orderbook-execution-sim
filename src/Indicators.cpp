//
// Created by mouad-sehli on 11/9/26.
//


#include "ob/Indicators.h"
#include <sstream>
#include <cstdint>

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
    }
    else {this->m_exists = false;}


    // store the current prices as the old ones
    this->m_prevAskPrice = currentBestAsk;
    this->m_prevBidPrice = currentBestBid;

}


std::string Indicators::toCsvLine(std::int64_t ts) const {

    std::stringstream os{};
    if (m_exists) {
        os << ts << ',' << this->m_spread << ',' << this->m_doubleMid << ',' << this->m_midChanged;
    } else {
        os << ts << ",,," << this->m_midChanged;
    }
    return os.str();
}