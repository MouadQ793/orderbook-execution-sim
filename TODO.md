## Todo 
these are changes and optimizations I aim to do after finishing the project.
### about part 1 
- add except for negative quantities in the parser before part 2.
- handle error if checknano in parsing.cpp is >9 
- too much if (!type.has_value()) return std::nullopt; turn it into a function
- add exceptions in partial treatement of orders when total is negative
- The invalid lines aren't really invalid but rather orders of type 5 which aren't studied in first version. to fix later
- time stamp sanity check, make sure the orders are well ordered, should not be an issue here because Lobster's file is sorted but good
sanity check to have