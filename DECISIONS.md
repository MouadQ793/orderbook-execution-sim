## Decisions
## Part 1 : Parsing
- MSFT data  for large-tick stock.
- Prices  will be int64_t type for exact comparisons and no rounding errors, quantities of orders as signed integers.
- Times stamps will be int64_t because int32_t would overflow.
- Continuous quantities use doubles.
- Tests : GoogleTest via fetch content for reproducibility.
- I decided to use camelCase notations for functions and variables and PascalCase for types like in learncpp.com

## Lobster
- helper function parseInt is in an unnamed namespace, so it is only visible in parsing.cpp. It extracts numbers under an optional shape, using std::from_chars with pointers to the first character and just past the last one.
- Prices and quantities are int64_t: sums and notional (price × quantity) can exceed 32 bits;
- An invalid line returns an empty std::optional; the converter prints and counts it.
- Fields not used by a message type keep their default value (0 or None as defined in  the header message).

## conversion 
- Unknown events are from orders placed before 9:30 which I don't have access to so they are ignored in the quantity tracker.
- a partial cancel becomes an M with the remaining quantity
- prices are divided by 100 to be ticks of 0.01
- timestamps are nanoseconds after 930 
- Types 5 (hidden executions), 6 (cross trades) and 7 (halts) produce no output line: they don’t change the visible book. Hidden executions are about 11% of executions by
- The backward time stamps sanity check found 3 issues where the lobster data had 12 digits fractions which produced unwanted behavior with timeToNano, I implemented truncation to fix, afterward 0 backward timestamps and the safety test passes. 

## Part 2 : Order Book 

### Structure
- OrderBook is a class with private containers: only the public functions related to the order types can change the book.
- Order / Level / Location are private nested structures.
- Each side is a std::map<price, Level>: bids sorted with "std::greater", and asks by default.
- Level also stores its price, duplicating the map key used for lookups.

### Interface
- add takes a complete 'OrderEvent', which we defined in the first part; cancel takes only the ID; modify and execute take both the ID and the quantity.
- Every operation returns an enum class instance, BookStatus, to report whether the order was incorporated properly.
- All critical checks are done before the operation is executed.

### Validation of operations
- Add: rejects types other than Add, duplicate IDs, non positive prices and quantities, and empty sides.
- Cancel: cancel and a full 'execute' share a private helper, removeOrder, which execute calls if the quantity consumes the whole order.
- Execute: executes the order; if the execution is bigger than the current quantity, it's rejected.
- Modify: receives the new quantity; if it's bigger than the current one, it moves the order down in the execution order.

### Data Inconsistency 
- Replaying the full MSFT day, the book becomes crossed at converted line 4,797 (best bid 31.08 >= best ask 31.08), with 0 messages rejected by the book, because the Lobster message file adds some orders and never removes them (around 2 % of the orders)
- It seems the orders are removed in the lobsterbook snapshot reference but without any messages doing so.
- Will handle them by making an "uncross rule" canceling orders when they cross.
- Uncross rule implemented when a new order crosses the opposite side, the opposite levels at crossing prices are removed.
- Uncross rule on the full MSFT day yields 2,645 stale orders removed and 0 crossings
- Invariant checker checks for 5 things, no empty lvl, totals equal queue sums, level prices equals key, index matches queues and book not crossed.


## Part 3 

- The indicators have their own class, which reads the order book by const reference, so it can't interfere with it and the book stays a pure reflection of the exchange.

### Spread, mid price

- Spread = best ask − best bid, in ticks. The mid is stored as the double mid (best bid + best ask, in half-ticks) to stay an integer.
- `midChanged` is true when a mid exists before and after the message and either best price changed. A side appearing or disappearing doesn't count, since no return can be computed.
- Price 0 means "no price": safe, because `add` rejects any price ≤ 0.
- Full MSFT day (see `python/part3_indicators.ipynb`): 13 rows with no mid, 3,784 mid changes (0.6% of messages).
| Spread | Share of messages | Share of time |
| --- | ---: | ---: |
| 1 tick | 80.3% | 99.2% |
| 2 ticks | 19.6% | 0.8% |
| 3 to 16 ticks | 0.03% | 0.02% |
- The spread is almost always 1 tick. When it opens to 2 ticks, it closes again almost at once, but those brief states generate a fifth of all messages (liquidity refill, and executions sharing a timestamp). Time-weighted statistics are needed: message counts overstate wide spreads.

- Added Depth public function in orderbook and imbalance computing in indicators, $I_1$ = the level 1 imbalance, and $I_{k=5}$ = the best 5 non empty level

### Queue position
- `queueAhead` returns the quantity ahead of an order at its own level only. It walks the queue, so it is the exact reference, not the O(1) indicator.
- It returns an optional, since 0 is a real answer (front of the queue).
- Each order gets an arrival ticket, renewed when a modify increases its quantity (it moves to the back). Comparing tickets says who is ahead in O(1), since `std::list` iterators have no index.
- `getOrderInfo` returns side, price, quantity and ticket in one lookup.
- `QueueTracker` follows a few tracked orders incrementally and reads the book by const reference. It is needed because the strategy's simulated orders (Part 4) will never be in the book.
- The tracker updates before `apply` for other orders (their old quantity is lost after) and after `apply` for the tracked order itself and for orders removed by the uncross rule.
- Stress test against `queueAhead`: 6.16M comparisons, 0 mismatches.