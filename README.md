The goal of this project is simulating SIMD instructions for 4‐bit integers using 64‐bit general-purpose registers and operations for Intel-AMD x86‐64 processors to try and speed up these low precision calculations through software-based vectorization.

The functions are in the uint4x16_t.cpp file, while the main.cpp file contains benchmarks to evaluate the performance of the functions. Also, the lookuptable.cpp file contains a possible alternate approach where we precompute the results and store them in a table. 

The project is meant to be compiled and executed on Linux.
