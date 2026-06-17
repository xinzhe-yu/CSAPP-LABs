# Cache Lab

In this lab, the student works on two C files called csim.c and
trans.c.  
There are two parts: 
- Part (a) involves implementing a cache
simulator in csim.c. 
- Part (b) involves writing a function that
computes the transpose of a given matrix in trans.c, with the goal of
minimizing the number of misses on a simulated cache.

# Part A: Writing a Cache Simulator 

The Cache Simulator takes a *valgrind* memory trace as input,simulates the hit/miss behavior of a cache memory on this trace, and outputs the total number of hits,
misses, and evictions.

The Cache is a small, fast memory that holds copies of data from larger, slower backing store, exploiting temporal and spatial locality to reduce average access latency. 

Structurally it's parameterized by (S, E, B): S = 2^s sets, E lines per set, B = 2^b bytes per block. An m-bit address splits into tag (t bits), set index (s bits), and block offset (b bits). Total capacity C = S × E × B. E determines associativity: E=1 is direct-mapped, E>1 is set-associative, one set with all lines is fully associative.

On access, the set index selects a set, tags are compared in parallel against the valid lines, and a match with valid=1 is a hit. Misses come in three flavors: compulsory (cold, first reference to a block), conflict (multiple blocks map to the same set and evict each other despite capacity remaining), and capacity (working set exceeds C). Direct-mapped caches suffer conflict misses that fully associative ones of the same size avoid, at the cost of comparator and lookup complexity.

## Implementation

[`csim.c`](https://github.com/you/repo/blob/main/csim.c) — set-associative cache simulator (S/E/B configurable).

## Result

![alt text]({32387924-0C90-4102-AB57-736F7081FEA1}.png)

Passes all eight test cases for full credit (27/27). Hit, miss, and eviction counts match the reference simulator exactly across every parameter configuration.

# Part B 