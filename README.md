# Comparative Analysis of Multiplication Algorithms

> **A high-performance benchmarking project analyzing the efficiency of Native (32/64-bit), Vedic, Long Multiplication, and Karatsuba algorithms.**

##  Project Overview
In high-performance computing and cryptography, selecting the optimal multiplication method is critical for efficiency. This project implements and compares three distinct categories of multiplication techniques: **Traditional Hardware** (Native 32/64-bit), **Ancient Mathematics** (Vedic), and **Arbitrary Precision** (Long Multiplication & Karatsuba).

The study benchmarks these algorithms based on input size, CPU cycle consumption, and algorithmic complexity, highlighting the specific "tipping points" where software-based arbitrary precision becomes necessary over hardware execution.

---

##  Algorithms Implemented

### 1. Native Hardware Multiplication (32-bit & 64-bit)
* **Mechanism:** Utilizes the CPU's built-in hardware multiplier by converting string inputs into fixed-size binary integers.
* **Pros:** Extremely fast (single machine instruction).
* **Cons:** Strictly limited by integer overflow. A 32-bit integer caps at ~4.2 billion, and 64-bit at ~18.4 quintillion.

### 2. Vedic Multiplication
* **Mechanism:** Based on the *Urdhva Tiryakbhyam* method. It treats numbers as reversed lists of digits and uses **lookup tables** for single-digit multiplication and addition to avoid calculation overhead.
* **Key Feature:** Uses a "diagonal sum" approach.

### 3. Long Multiplication
* **Mechanism:** A software simulation of the classic "pen-and-paper" grade-school method.
* **Capability:** Arbitrary precision (limited only by memory).
* **Complexity:** $O(n^2)$ complexity makes it inefficient for very large inputs.

### 4. Karatsuba Algorithm
* **Mechanism:** A "Divide and Conquer" algorithm that splits large numbers into halves to reduce the total number of multiplications from 4 to 3 per step.
* **Complexity:** Approx $O(n^{1.585})$.
* **Use Case:** Designed for massive numbers (thousands of digits). In this specific benchmark range (up to 40 digits), its overhead made it slower than Vedic methods.

---

##  Methodology

To ensure accurate performance metrics, the project employed low-level CPU timing:

* **Measurement:** Used `rdtsc()` (Time Stamp Counter) to measure exact CPU cycles.
* **Optimization Control:** Utilized `volatile` variables to prevent compiler optimizations from skipping calculations during benchmark loops.
* **Testing Protocol:** Each multiplication was executed 1,000 to 10,000 times to obtain a stable average cycle count, filtering out OS noise.

---
