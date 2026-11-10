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
    static constexpr std::string_view csvHeader{"timestamp,spread,double_mid,mid_changed,imbalance_1,imbalance_k,ret_mean,ret_vol,spread_mean,depth_mean,intensity,ofi"};
    double getImbalance1() const {return m_imbalance1;}
    double getImbalanceK() const {return m_imbalancek;}
    double getRetMean() const {return m_mean;}
    double getRetVariance() const {return m_variance;}
    double getSpreadMean() const {return m_spreadMean;}
    double getDepthMean() const {return m_depthMean;}
    double getIntensity() const {return m_lambda;}
    std::int64_t getOfi() const {return m_ofi;}

private :
    //stats
    std::int64_t m_prevBidPrice{0};
    std::int64_t m_prevAskPrice{0};
    std::int64_t m_prevTimestamp{0};
    double m_variance{0};
    double m_mean{0};
    static constexpr double m_alpha{0.05};
    std::int64_t m_prevDoubleMid{0};
    double m_spreadMean{0};
    static constexpr double tau{1e9};
    std::int64_t m_depth{};
    double m_depthMean{};
    double m_lambda {};
    std::int64_t m_ofi{};
    std::int64_t m_prevBestAskQty{0};
    std::int64_t m_prevBestBidQty{0};


    bool m_exists{};
    double m_imbalance1{};
    double m_imbalancek{};
    static constexpr std::int64_t imbalanceDepth{5};
    std::int64_t m_spread{};
    std::int64_t m_doubleMid{};
    bool m_midChanged{};
    static double imbalance(std::int64_t bidQty, std::int64_t askQty);
    static double compute_a(std::int64_t time);

};







