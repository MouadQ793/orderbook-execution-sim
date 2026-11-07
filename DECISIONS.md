## Decisions
## Parsing
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
- Unkown events are from orders placed before 9:30 which I don't have access to so they are ignored in the quantity tracker.
- a partial cancel becomes an M with the remaining quantity
- prices are divided by 100 to be ticks of 0.01
- timestamps are nanoseconds after 930 
- Types 5 (hidden executions), 6 (cross trades) and 7 (halts) produce no output line: they don’t change the visible book. Hidden executions are about 11% of executions by
- The backward time stamps sanity check found 3 issues where the lobster data had 12 digits fractions which produced unwanted behavior with timeToNano, I implemented truncation to fix, afterward 0 backward timestamps and the safety test passes. 