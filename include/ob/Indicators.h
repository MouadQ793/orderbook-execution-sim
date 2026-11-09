#pragma once
#include <cstdint>
#include "OrderBook.h"





class Indicators {
public :

    std::int64_t get_Spread() const {return m_spread;}
    std::int64_t get_doublemid() const {return m_doubleMid;}
    bool get_midchanged() const {return m_midChanged;}
    bool get_exists () const {return m_exists;}
    void update(const OrderBook& book, const OrderEvent& event);

private :
    std::optional<std::int64_t> m_prevBidPrice{};
    std::optional<std::int64_t> m_prevAskPrice{};
    bool m_exists{};

    std::int64_t m_spread{};
    std::int64_t m_doubleMid{};
    bool m_midChanged{};

};







