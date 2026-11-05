

## Decisions 
- MSFT data  for large-tick stock. \
- Prices  will be int64_t type for exact comparisons and no rounding errors, quantities of orders as signed integers. \
- Times stamps will be int64_t because int32_t would overflow.\
- Continuous quantities use doubles.
- Tests : GoogleTest via fetch content for reproducibility. 
- I decided to use camelCase notations for functions and variables and PascalCase for types like in learncpp.com