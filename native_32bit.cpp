#include <iostream>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <immintrin.h> // For _mm_lfence and __rdtscp

#ifdef _MSC_VER
#include <windows.h>
#include <intrin.h> // For __rdtscp on MSVC
#endif

// A function to read the CPU's time-stamp counter.
static inline uint64_t rdtsc() {
    unsigned int aux;
    return __rdtscp(&aux);
}

int main() {
    // These settings help make the benchmark more stable by reducing OS interference.
#ifdef _MSC_VER
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    SetThreadAffinityMask(GetCurrentThread(), 1);
#endif

    std::string a_str, b_str;
    std::cout << "Enter two numbers separated by a space: ";
    std::cin >> a_str >> b_str;

    // First, try converting once to ensure the input is valid for 32-bits.
    try {
        (void)std::stoul(a_str); // Use stoul for unsigned long (typically 32-bit)
        (void)std::stoul(b_str);
    } catch (const std::exception&) {
        // Updated error message for 32-bit integers.
        std::cout << "\nERROR: Input is invalid or too large for a 32-bit integer.\n";
        return 1;
    }

    constexpr int LOOP = 1000;
    // The result variable is now a 32-bit integer.
    volatile uint32_t result = 0; 
    
    _mm_lfence(); // Prevents reordering of instructions by the CPU
    uint64_t start = rdtsc();

    for (int i = 0; i < LOOP; ++i) {
        // Step 1: Convert strings to 32-bit binary integers INSIDE the loop.
        uint32_t num1 = std::stoul(a_str);
        uint32_t num2 = std::stoul(b_str);

        // Step 2: Perform the native 32-bit multiplication.
        result = num1 * num2;
    }

    _mm_lfence();
    uint64_t end = rdtsc();

    // --- Calculate and Print Results ---
    double total_cycles = static_cast<double>(end - start);
    double avg_cycles_per_operation = total_cycles / LOOP;

    std::cout << "\n--- Benchmark (Averaged over " << LOOP << " runs) ---\n";
    
    std::cout << "\n## Total Cycles for Combined 32-bit Operation ##" << std::endl;
    std::cout << "Total cycles for " << LOOP << " operations: " << total_cycles << std::endl;
    std::cout << "Average cycles per operation: " << avg_cycles_per_operation << std::endl;
    std::cout << "Result of last operation: " << result << " (Note: Result may have overflowed)" << std::endl;

    return 0;
}