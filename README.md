# orderbook-execution-sim

A limit order book and event-driven execution simulator in C++, built on NASDAQ order-level data (LOBSTER).
The end goal is to compare an adaptive execution strategy (passive vs aggressive, driven by fill probability,
order book imbalance and adverse selection) against a TWAP benchmark.

## Status

| Part | Content | Status |
| --- | --- | --- |
| 1 | Data: parsing, LOBSTER conversion, tests | Done |
| 2 | Order book (price levels, FIFO queues, O(1) cancel), invariant checker, full-day replay | Done |
| 3 | Microstructure indicators (spread, imbalance, queue position, EWMA) | In progress |
| 4 | Event-driven engine with latency and simulated fills | Planned |
| 5 | Execution strategy vs TWAP (fill model, markouts, implementation shortfall) | Planned |
| 6 | Performance measurement and optimisation | Planned |

## Data

The project uses the free LOBSTER sample for **MSFT on 21 June 2012** (level 10), available at
[lobsterdata.com](https://lobsterdata.com/info/DataSamples.php). The data is not included in this repository:
download it and place the message file in `Data/`.

MSFT was chosen because it behaves as a **large-tick** stock (one-tick spread most of the time, long queues),
which makes it a good equity analogue of government bond futures such as the Bund.

## Message format

LOBSTER messages are converted once into the project's own format, which is the only format the rest of the
code reads:

```
timestamp,type,order_id,side,price,quantity
13994120,A,16116348,S,3105,100
20823678,C,16116815,,,
```

- `timestamp`: nanoseconds since 9:30
- `type`: `A` add, `C` cancel, `E` execute, `M` modify (new remaining quantity)
- `price`: integer ticks of $0.01
- unused fields are left empty

## Part 1 results (full MSFT day)

| | Lines |
| --- | ---: |
| Read | 668,765 |
| Written | 663,288 |
| of which `A` / `C` / `E` / `M` | 329,566 / 298,821 / 29,393 / 5,508 |
| Skipped: events on orders placed before 9:30, hidden executions | 5,477 |

A timestamp-order check on the converted file found 3 LOBSTER timestamps with 12-digit fractions
(floating-point printing artifacts in the source file). Fractions are now truncated to 9 digits
(error ≤ 1 ns), and the converted file has no backward timestamps.

## Part 2: the order book

The book **mirrors** the exchange feed: it applies each message (add, cancel, execute, modify) and never matches
orders itself.

- **Price levels:** `std::map` per side, bids sorted in descending order, so the best price is always `begin()`.
- **FIFO queue per level:** `std::list`, so cancelling an order anywhere in the queue is O(1) and iterators stay valid.
- **Index by order ID:** `std::unordered_map` from ID to (side, price, iterator), giving O(1) average cancel,
  execute and modify.
- **Validation before mutation:** every message is checked first (unknown or duplicate ID, invalid side, price or
  quantity, execution larger than the order). A rejected message leaves the book unchanged and returns a status.
- **Modify:** a decrease keeps queue priority; an increase moves the order to the back of its queue.

### Data inconsistency: stale orders

About 2% of the orders added in the LOBSTER message file are never removed by any later message, although
LOBSTER's own book snapshots show them gone. Replaying messages only, these stale orders eventually cross the book.
**Uncross rule:** when a new order reaches the opposite side, the stale opposite levels it crosses are removed.

### Results (full MSFT day)

| | |
| --- | ---: |
| Messages replayed | 663,288 |
| Stale orders removed by the uncross rule | 2,645 |
| Crossed books after the rule | 0 |
| Invariant checks (every 10,000 messages + end of day) | 67, no violation |
| Unit tests | 46, all passing |

The invariant checker verifies five rules: no empty level, each level's total equals the sum of its queue, each
level's price equals its key, the index and the queues agree exactly, and the book is never crossed.

Known limit: stale orders that are never crossed stay in the book. Part 4 will measure their effect.

Design choices are recorded in [DECISIONS.md](DECISIONS.md), known limits in [TODO.md](TODO.md).

## Build and run

Requires CMake ≥ 3.20 and a C++20 compiler. GoogleTest is downloaded automatically by CMake.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/tests
./build/converter    # LOBSTER message file -> Data/converted_book_lobster.csv
./build/replay       # replays the converted file through the order book and checks invariants
```

Run the programs from the project root, so the relative paths to `Data/` resolve.

## Disclosure: use of AI

This project is also how I am learning C++, and I used Claude (Anthropic) as a tutor throughout.

- **Project design:** the project plan and a step-by-step learning guide (concepts, tasks and hints for each question) were prepared with Claude.
- **Workflow:** for each task, Claude points me to the relevant sections of learncpp.com and cppreference.com. After reading them, I design and write my own solution, then send it to Claude for review; it points out potential bugs, which I then fix myself.
- **Code:** the large majority of the C++ code (parsing, LOBSTER conversion, the order book, the invariant checker and the replay) was written by me. Claude provided a few pieces: mainly the `parseInt` helper based on `std::from_chars`, a first example test for `fitOrderEvent` used as a template for the others, part of `print` formatting of the book, part of the order book unit tests ( modify, apply and the uncross rule), the `CMakeLists.txt` configuration and this README.
  - The tests started getting too redundant so started 

Every design choice and its reason is recorded in [DECISIONS.md](DECISIONS.md).