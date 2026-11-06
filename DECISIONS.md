## Decisions
- MSFT data  for large-tick stock.
- Prices  will be int64_t type for exact comparisons and no rounding errors, quantities of orders as signed integers.
- Times stamps will be int64_t because int32_t would overflow.
- Continuous quantities use doubles.
- Tests : GoogleTest via fetch content for reproducibility.
- I decided to use camelCase notations for functions and variables and PascalCase for types like in learncpp.com

## 2
- helper function parseInt is in an unnamed namespace, so it is only visible in parsing.cpp. It extracts numbers under an optional shape, using std::from_chars with pointers to the first character and just past the last one.
- Prices and quantities are int64_t: sums and notional (price × quantity) can exceed 32 bits;
- An invalid line returns an empty std::optional; the converter prints and counts it.
- Fields not used by a message type keep their default value (0 or None as defined in  the header message).