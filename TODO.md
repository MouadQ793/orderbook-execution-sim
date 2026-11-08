## Todo 
these are changes and optimizations I aim to do after finishing the project.
## Part 1 : Parsing
- add except for negative quantities in the parser before part 2.
- handle error if checknano in parsing.cpp is >9 
- too much if (!type.has_value()) return std::nullopt; turn it into a function
- add exceptions in partial treatement of orders when total is negative
- The invalid lines aren't really invalid but rather orders of type 5 which aren't studied in first version. to fix later
- Fix wraparound of splitOrder, not urgent because LobsterData is clean.
## Part 2 : OrderBook
- According to claude there is a more efficient way to remove the duplicate part in BookStatus::Add, to do later. 
- Simplify the tests by making a function for repititve tasks