//
// Created by mouad-sehli on 11/11/26.
//

#include "../include/ob/Engine.h"

#include <iostream>
#include <fstream>
#include "ob/OrderBook.h"
#include "ob/parsing.h"

Engine::Engine(const std::string& path)
    : m_input{path}
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
                // notify subscribers
                pushNextMarketEvent();
                break;
            case Priority::Strategy:
                // apply strategy later
                break;
            default:
                std::cerr << " Engine::Run, Unknown Priority";
                return;

        }
    }
}








