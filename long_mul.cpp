#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm> // Required for std::reverse
#include <immintrin.h>

#ifdef _MSC_VER
#include <windows.h>
#include <intrin.h>
#endif

// Function to measure CPU cycles
static inline uint64_t rdtsc() {
    unsigned int aux;
    return __rdtscp(&aux);
}

// Multiplies two numbers represented as strings
std::string multiplyDecimal(const std::string &num1, const std::string &num2) {
    if (num1 == "0" || num2 == "0") return "0";

    int n = num1.size();
    int m = num2.size();
    std::vector<int> res(n + m, 0);

    // Multiply digit by digit from right to left
    for (int i = n - 1; i >= 0; --i) {
        for (int j = m - 1; j >= 0; --j) {
            int mul = (num1[i] - '0') * (num2[j] - '0');
            int sum = res[i + j + 1] + mul;
            res[i + j + 1] = sum % 10;
            res[i + j] += sum / 10;
        }
    }

    // Convert the result vector back to a string, skipping leading zeros
    std::string result_str;
    bool leading_zero = true;
    for (int digit : res) {
        if (digit == 0 && leading_zero) continue;
        leading_zero = false;
        result_str += std::to_string(digit);
    }

    return result_str.empty() ? "0" : result_str;
}

int main() {
#ifdef _MSC_VER
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    SetThreadAffinityMask(GetCurrentThread(), 1);
#endif

    std::string a, b;
    std::cin >> a >> b;

    std::string result;
    constexpr int LOOP = 1000;

    _mm_lfence();
    uint64_t start = rdtsc();
    for (int i = 0; i < LOOP; ++i) {
        result = multiplyDecimal(a, b);
    }
    _mm_lfence();
    uint64_t end = rdtsc();

    double total = double(end - start);
    double avg = total / LOOP;

    // Output in the format the GUI expects
    std::cout << "\nResult (last run): " << result << "\n";
    std::cout << "Total cycles: " << total << "\n";
    std::cout << "Avg cycles per multiply: " << avg << "\n";

    return 0;
}
