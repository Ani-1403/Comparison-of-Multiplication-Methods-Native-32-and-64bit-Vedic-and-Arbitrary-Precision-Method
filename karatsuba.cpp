#define NOMINMAX

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>
#include <immintrin.h>

#ifdef _MSC_VER
#include <intrin.h>
#include <windows.h>
#endif

static inline uint64_t rdtsc() {
    unsigned int aux;
    return __rdtscp(&aux);
}

// Convert string to vector<uint8_t> (reversed)
std::vector<uint8_t> toDigits(const std::string &s) {
    std::vector<uint8_t> res(s.size());
    for (int i = 0; i < s.size(); ++i)
        res[s.size() - 1 - i] = s[i] - '0';
    return res;
}

// Convert digits vector to string (removing leading zeros)
std::string toString(const std::vector<uint8_t> &digits) {
    std::string s;
    bool leading = true;
    for (int i = digits.size() - 1; i >= 0; --i) {
        if (leading && digits[i] == 0) continue;
        leading = false;
        s += char('0' + digits[i]);
    }
    return s.empty() ? "0" : s;
}

// Adds two vectors (a += b)
void add(std::vector<uint8_t> &a, const std::vector<uint8_t> &b) {
    int n = std::max(a.size(), b.size());
    a.resize(n, 0);
    int carry = 0;
    for (int i = 0; i < n || carry; ++i) {
        if (i == a.size()) a.push_back(0);
        int sum = a[i] + (i < b.size() ? b[i] : 0) + carry;
        a[i] = sum % 10;
        carry = sum / 10;
    }
}

// Subtracts two vectors (a -= b), assumes a >= b
void subtract(std::vector<uint8_t> &a, const std::vector<uint8_t> &b) {
    int borrow = 0;
    for (int i = 0; i < a.size(); ++i) {
        int sub = a[i] - (i < b.size() ? b[i] : 0) - borrow;
        if (sub < 0) {
            sub += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        a[i] = sub;
    }
    while (a.size() > 1 && a.back() == 0) a.pop_back();
}

// Multiplies two small vectors directly
std::vector<uint8_t> basicMul(const std::vector<uint8_t> &a, const std::vector<uint8_t> &b) {
    std::vector<uint8_t> res(a.size() + b.size());
    for (int i = 0; i < a.size(); ++i) {
        for (int j = 0; j < b.size(); ++j) {
            res[i + j] += a[i] * b[j];
            if (res[i + j] >= 10) {
                res[i + j + 1] += res[i + j] / 10;
                res[i + j] %= 10;
            }
        }
    }
    while (res.size() > 1 && res.back() == 0) res.pop_back();
    return res;
}

// Karatsuba multiplication
std::vector<uint8_t> karatsuba(const std::vector<uint8_t> &a, const std::vector<uint8_t> &b) {
    int n = std::max(a.size(), b.size());
    if (n <= 32) return basicMul(a, b);  // fallback for small inputs

    int half = n / 2;

    std::vector<uint8_t> a0(a.begin(), a.begin() + std::min<int>(a.size(), half));
    std::vector<uint8_t> a1(a.begin() + std::min<int>(a.size(), half), a.end());
    std::vector<uint8_t> b0(b.begin(), b.begin() + std::min<int>(b.size(), half));
    std::vector<uint8_t> b1(b.begin() + std::min<int>(b.size(), half), b.end());

    std::vector<uint8_t> p0 = karatsuba(a0, b0);
    std::vector<uint8_t> p1 = karatsuba(a1, b1);

    std::vector<uint8_t> sumA = a0;
    std::vector<uint8_t> sumB = b0;
    add(sumA, a1);
    add(sumB, b1);

    std::vector<uint8_t> p2 = karatsuba(sumA, sumB);
    subtract(p2, p0);
    subtract(p2, p1);

    std::vector<uint8_t> result(p0.size() + 2 * half, 0);
    for (int i = 0; i < p0.size(); ++i) result[i] += p0[i];
    for (int i = 0; i < p2.size(); ++i) result[i + half] += p2[i];
    for (int i = 0; i < p1.size(); ++i) result[i + 2 * half] += p1[i];

    for (int i = 0; i + 1 < result.size(); ++i) {
        if (result[i] >= 10) {
            result[i + 1] += result[i] / 10;
            result[i] %= 10;
        }
    }
    while (result.size() > 1 && result.back() == 0) result.pop_back();
    return result;
}

int main() {
    using namespace std;

    #ifdef _MSC_VER
    HANDLE hProc = GetCurrentProcess();
    HANDLE hThread = GetCurrentThread();
    SetPriorityClass(hProc, REALTIME_PRIORITY_CLASS);
    SetThreadAffinityMask(hThread, DWORD_PTR{1});
    #endif

    string s1, s2;
    cout << "Enter first number (up to 40+ digits): "; cin >> s1;
    cout << "Enter second number (up to 40+ digits): "; cin >> s2;

    auto a = toDigits(s1);
    auto b = toDigits(s2);
    constexpr int TRIALS = 1000;
    uint64_t total = 0, best = UINT64_MAX;
    vector<uint8_t> result;

    for (int t = 0; t < TRIALS; ++t) {
        _mm_lfence();
        uint64_t start = rdtsc();
        result = karatsuba(a, b);
        _mm_lfence();
        uint64_t end = rdtsc();

        uint64_t cycles = end - start;
        total += cycles;
        best = min(best, cycles);
    }

    cout << "\nProduct: " << toString(result) << "\n";
    cout << "Best CPU Cycles   : " << best << "\n";
    cout << "Average CPU Cycles: " << (total / TRIALS) << "\n";
    return 0;
}
