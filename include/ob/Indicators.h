#pragma once
#include <cstdint>
#include "OrderBook.h"
#include <string_view>




class Indicators {
public :

    std::int64_t get_Spread() const {return m_spread;}
    std::int64_t get_doublemid() const {return m_doubleMid;}
    bool get_midchanged() const {return m_midChanged;}
    bool get_exists () const {return m_exists;}
    void update(const OrderBook& book, const OrderEvent&);
    std::string toCsvLine(std::int64_t ts) const;
    static constexpr std::string_view csvHeader{"timestamp,spread,double_mid,mid_changed"};

private :
    std::optional<std::int64_t> m_prevBidPrice{0};
    std::optional<std::int64_t> m_prevAskPrice{0};
    bool m_exists{};

    std::int64_t m_spread{};
    std::int64_t m_doubleMid{};
    bool m_midChanged{};


};







