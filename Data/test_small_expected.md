## Small exepected sample

### 1. `0,A,1,B,100,50`
- Best bid: 100 (total 50) · Best ask: none
- Bids: 100 → [#1:50]
- Asks: empty

### 2. `1077,A,2,S,102,150`
- Best bid: 100 (total 50) · Best ask: 102 (total 150)
- Bids: 100 → [#1:50]
- Asks: 102 → [#2:150]

### 3. `2032,E,1,,,30`
- Best bid: 100 (total 20) · Best ask: 102 (total 150)
- Bids: 100 → [#1:20]
- Asks: 102 → [#2:150]

### 4. `3103,C,2,,,`
- Best bid:100 (total 20) . Best ask : -- 
- Bids: 100 -> [#1:20]

### 5. `3992,M,1,,,50`
- Best bid : 100(total 50) . Best ask : --
- Bids: 100 -> [#1:50]

### 6. `5045,A,3,S,105,200`
- Best bid: 100 (total 50) · Best ask: 105 (total 200)
- Bids: 100 → [#1:50]
- Asks: 105 → [#3:200]

### 7. `5970,A,4,S,105,120`
- Best bid: 100 (total 50) · Best ask: 105 (total 320)
- Bids: 100 → [#1:50]
- Asks: 105 → [#3:200, #4:120]

### 8. `7917,E,3,,,200`
- Best bid: 100 (total 50) · Best ask: 105 (total 120)
- Bids: 100 → [#1:50]
- Asks: 105 → [#4:120]

### 9. `7917,E,4,,,60`
- Best bid: 100 (total 50) · Best ask: 105 (total 60)
- Bids: 100 → [#1:50]
- Asks: 105 → [#4:60]

### 10. `10056,M,4,,,30`
- Best bid: 100 (total 50) · Best ask: 105 (total 30)
- Bids: 100 → [#1:50]
- Asks: 105 → [#4:30]

### 11. `11565,A,5,B,95,100`
- Best bid: 100 (total 50) · Best ask: 105 (total 30)
- Bids: 95 -> [#5:100] . 100 → [#1:50]
- Asks: 105 → [#4:30]

### 12. `13475,E,1,,,30`
- Best bid: 100 (total 20) · Best ask: 105 (total 30)
- Bids: 100 → [#1:20] . 95 -> [#5:100]
- Asks: 105 → [#4:30]

### 13. `13475,C,12,,,`
- Best bid: 100 (total 20) · Best ask: 105 (total 30)
- Bids: 100 → [#1:20] . 95 -> [#5:100] 
- Asks: 105 → [#4:30]
  (12 doesn't exist)

### 14. `13475,C,1,,,`
- Best bid: 95 (total 100) · Best ask: 105 (total 30)
- Bids: 95 → [#5:100]
- Asks: 105 → [#4:30]