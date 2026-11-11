//
// Created by mouad-sehli on 11/11/26.
//

#pragma once
#include <cstdint>
#include <queue>
#include <tuple>
#include <vector>

#include "message.h"


enum class Priority {
    // This is so Market orders go through before execution ones
    Market,
    Strategy
};


struct ScheduledEvent {
    std::int64_t timestamp;
    Priority priority;
    std::int64_t ticket;
    OrderEvent orderEvent;
    // Claude suggested to define == like this but I kept my implementation a bit lower
    // bool operator==(const ScheduledEvent&) const = default;
    // my way might cause a problem if the tickets are not unique because it doesn't compare the underlying order
};


// inline bool operator<(const ScheduledEvent &schedEventA, const ScheduledEvent &schedEventB) {
//     if (schedEventA.timestamp < schedEventB.timestamp) return true;
//     if (schedEventA.timestamp == schedEventB.timestamp) {
//         if (schedEventA.priority < schedEventB.priority) return true;
//         if (schedEventA.priority == schedEventB.priority) {
//             if (schedEventA.ticket < schedEventB.ticket) return true;
//             if (schedEventA.ticket > schedEventB.ticket) return false;
//             return false;
//         }
//         return false;
//     }
//     return false;
// }
// This was my first suggestion, but claude informed me there is a way more optimal way to do it that takes one line which I took




inline bool operator<(const ScheduledEvent &a, const ScheduledEvent &b) {
    return std::tie(a.timestamp, a.priority, a.ticket) < std::tie(b.timestamp, b.priority, b.ticket);
}
inline bool operator==(const ScheduledEvent &a, const ScheduledEvent &b) {
    return std::tie(a.timestamp, a.priority, a.ticket) == std::tie(b.timestamp, b.priority, b.ticket);
}


inline bool operator>(const ScheduledEvent& a, const ScheduledEvent& b) { return b < a; }

using EventQueue = std::priority_queue<ScheduledEvent, std::vector<ScheduledEvent>, std::greater<ScheduledEvent>>;