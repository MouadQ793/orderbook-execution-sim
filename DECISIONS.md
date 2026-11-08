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

## Data Inconsistency 
- Replaying the full MSFT day, the book becomes crossed at converted line 4,797 (best bid 31.08 >= best ask 31.08), with 0 messages rejected by the book, because the Lobster message file adds some orders and never removes them (around 2 % of the orders)
- It seems the orders are removed in the lobsterbook snapshot reference but without any messages doing so.
- Will handle them by making an "uncross rule" canceling orders when they cross.
- Uncross rule implemented when a new order crosses the opposite side, the opposite levels at crossing prices are removed.
- Uncross rule on the full MSFT day yields 2,645 stale orders removed and 0 crossings
- Invariant checker checks for 5 things, no empty lvl, totals equal queue sums, level prices equals key, index matches queues and book not crossed.