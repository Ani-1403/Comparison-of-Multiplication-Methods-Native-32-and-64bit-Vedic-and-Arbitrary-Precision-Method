#include <iostream>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <immintrin.h>

#ifdef _MSC_VER
#include <windows.h>
#include <intrin.h>
#endif

static inline uint64_t rdtsc() {
    unsigned int aux;
    return __rdtscp(&aux);
}

int main() {
#ifdef _MSC_VER
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    SetThreadAffinityMask(GetCurrentThread(), 1);
#endif

    std::string a_str, b_str;
    std::cout << "Enter two numbers separated by a space: ";
    std::cin >> a_str >> b_str;

    // First, try converting once to ensure the input is valid before benchmarking.
    try {
        // FIX APPLIED HERE:
        (void)std::stoull(a_str);
        (void)std::stoull(b_str);
    } catch (const std::exception&) {
        std::cout << "\nERROR: Input is invalid or too large for a 64-bit integer.\n";
        return 1;
    }

    constexpr int LOOP = 1000;
    volatile uint64_t result = 0;
    
    _mm_lfence();
    uint64_t start = rdtsc();

    for (int i = 0; i < LOOP; ++i) {
        uint64_t num1 = std::stoull(a_str);
        uint64_t num2 = std::stoull(b_str);
        result = num1 * num2;
    }

    _mm_lfence();
    uint64_t end = rdtsc();

    double total_cycles = static_cast<double>(end - start);
    double avg_cycles_per_operation = total_cycles / LOOP;

    std::cout << "\n--- Benchmark (Averaged over " << LOOP << " runs) ---\n";
    std::cout << "\n## Total Cycles for Combined Operation (Conversion + Multiplication) ##" << std::endl;
    std::cout << "Total cycles for " << LOOP << " operations: " << total_cycles << std::endl;
    std::cout << "Average cycles per operation: " << avg_cycles_per_operation << std::endl;
    std::cout << "Result of last operation: " << result << " (Note: Result may have overflowed)" << std::endl;

    return 0;
}