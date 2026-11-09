#pragma once
#include <cstdint>
#include "OrderBook.h"
#include <string_view>
#include <string>



class Indicators {
public :

    std::int64_t getSpread() const {return m_spread;}
    std::int64_t getdoublemid() const {return m_doubleMid;}
    bool getmidchanged() const {return m_midChanged;}
    bool getexists () const {return m_exists;}
    void update(const OrderBook& book, const OrderEvent&);
    std::string toCsvLine(std::int64_t ts) const;
    static constexpr std::string_view csvHeader{"timestamp,spread,double_mid,mid_changed,imbalance_1,imbalance_k"};


private :
    std::int64_t m_prevBidPrice{0};
    std::int64_t m_prevAskPrice{0};
    bool m_exists{};

    double m_imbalance1{};
    double m_imbalancek{};
    static constexpr std::int64_t imbalanceDepth{5};
    std::int64_t m_spread{};
    std::int64_t m_doubleMid{};
    bool m_midChanged{};
    static double imbalance(std::int64_t bidQty, std::int64_t askQty);


};







