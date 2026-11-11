//
// Created by mouad-sehli on 11/11/26.
//

#include "../include/ob/Engine.h"

#include <iostream>
#include <fstream>
#include "ob/OrderBook.h"
#include "ob/parsing.h"

Engine::Engine(const std::string& path, std::int64_t latency)
    : m_input{path}
    , m_latency{latency}
{

    if (!m_input) {
        std::cerr << "Error, Engine couldn't read path " << path << std::endl ;
        return;
    }
    std::string orderInput{};
    std::getline(m_input, orderInput);  // Pop first line
    pushNextMarketEvent();
}


bool Engine::pushNextMarketEvent() {
    std::string orderInput{};
    while (std::getline(m_input, orderInput)) {
        // read input
        std::array<std::string_view, fieldCount> fields{splitOrder(orderInput)};
        std::optional<OrderEvent> event{fitOrderEvent(fields)};
        // if valid event then stores and push the next scheduled event
        // to the event Queue
        if (!event) {
            continue;
        }
        m_eventQueue.push(ScheduledEvent{event->timestamp, Priority::Market, m_nextTicket++, *event});
        return true;
    }
    return false;
}


void Engine::run() {
    //
    while (!m_eventQueue.empty()) {
        ScheduledEvent nextScheduledEvent{this->m_eventQueue.top()};

        this->m_eventQueue.pop();

        // move ts to this moment
        this->m_currentTime = nextScheduledEvent.timestamp;

        switch (nextScheduledEvent.priority) {
            case Priority::Market:
                m_orderBook.apply(nextScheduledEvent.orderEvent); // apply the event
                for (const auto& subscribed : m_subscribers) {
                    subscribed(m_orderBook, nextScheduledEvent.orderEvent);
                }
                pushNextMarketEvent();
                break;
            case Priority::Strategy:
                for (const auto& handler : m_strategyHandlers) {
                    handler(m_orderBook, nextScheduledEvent.orderEvent);
                }
                // apply strategy later
                break;
            default:
                std::cerr << " Engine::Run, Unknown Priority";
                return;

        }
    }
}

void Engine::subscribe(const BookCallBack& callBack) {
   m_subscribers.push_back(callBack);
}

bool Engine::schedule(std::int64_t timestamp, const OrderEvent& event) {
    if (this->m_currentTime > timestamp) {
        return false;
    }
    ScheduledEvent nextScheduledEvent{timestamp, Priority::Strategy, this->m_nextTicket++, event};
    this->m_eventQueue.push(nextScheduledEvent);
    return true;
}

bool Engine::send(const OrderEvent& event) {
    OrderEvent delayedEvent{m_currentTime + m_latency, event.type, event.orderId, event.side, event.price, event.quantity};
    return schedule(m_currentTime + m_latency, delayedEvent);
}

void Engine::onStrategyEvent(const BookCallBack& callback) {
    m_strategyHandlers.push_back(callback);
}



