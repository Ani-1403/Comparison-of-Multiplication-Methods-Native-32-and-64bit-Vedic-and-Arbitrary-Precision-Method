#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cctype>
#include <cstdint>
#include <algorithm>
#include <immintrin.h>

#ifdef _MSC_VER
  #define NOMINMAX
  #include <windows.h>
  #include <intrin.h>
#endif

static inline uint64_t rdtsc() {
    unsigned int aux;
    return __rdtscp(&aux);
}

// Tables for single-digit multiply and add
alignas(32) static const uint8_t lookupMul[10][10] = {
    {0,0,0,0,0,0,0,0,0,0},
    {0,1,2,3,4,5,6,7,8,9},
    {0,2,4,6,8,10,12,14,16,18},
    {0,3,6,9,12,15,18,21,24,27},
    {0,4,8,12,16,20,24,28,32,36},
    {0,5,10,15,20,25,30,35,40,45},
    {0,6,12,18,24,30,36,42,48,54},
    {0,7,14,21,28,35,42,49,56,63},
    {0,8,16,24,32,40,48,56,64,72},
    {0,9,18,27,36,45,54,63,72,81}
};

static const uint8_t addTable[10][10] = {
    {0,1,2,3,4,5,6,7,8,9},
    {1,2,3,4,5,6,7,8,9,10},
    {2,3,4,5,6,7,8,9,10,11},
    {3,4,5,6,7,8,9,10,11,12},
    {4,5,6,7,8,9,10,11,12,13},
    {5,6,7,8,9,10,11,12,13,14},
    {6,7,8,9,10,11,12,13,14,15},
    {7,8,9,10,11,12,13,14,15,16},
    {8,9,10,11,12,13,14,15,16,17},
    {9,10,11,12,13,14,15,16,17,18}
};

int carryCount = 0;

void vedicAdd(std::vector<uint16_t>& res, int idx, uint32_t val) {
    bool carry = false;
    while (val > 0 || carry) {
        uint32_t digit = val % 10;
        uint8_t sum = addTable[res[idx]][digit];
        carry = (sum >= 10);
        if (carry) ++carryCount;
        res[idx] = sum % 10;
        val = val / 10 + (carry ? 1 : 0);
        ++idx;
    }
}

int main() {
    using namespace std;

    #ifdef _MSC_VER
      HANDLE hProc = GetCurrentProcess();
      HANDLE hThread = GetCurrentThread();
      SetPriorityClass(hProc, REALTIME_PRIORITY_CLASS);
      SetThreadAffinityMask(hThread, DWORD_PTR{1});
    #endif

    constexpr int TRIALS = 10000;
    struct RoundResult {
        string a, b, product;
        int mulCount, baseAddCount, carryAddCount;
        double avgCycles, avgMulCycles, avgAddCycles;
        uint64_t bestTotalOpCycles;
    };

    vector<RoundResult> results;
    results.reserve(5);

    for (int round = 1; round <= 5; ++round) {
        string s1, s2;
        auto valid = [](const string &s) {
            return !s.empty() && s.size() <= 20 && all_of(s.begin(), s.end(), ::isdigit);
        };
        do { cout << "Round " << round << " - first number: "; cin >> s1; } while (!valid(s1));
        do { cout << "Round " << round << " - second number: "; cin >> s2; } while (!valid(s2));

        int n = s1.size(), m = s2.size();
        vector<uint8_t> A(n), B(m);
        for (int i = 0; i < n; ++i) A[n - 1 - i] = s1[i] - '0';
        for (int i = 0; i < m; ++i) B[m - 1 - i] = s2[i] - '0';

        vector<uint16_t> res(n + m);
        carryCount = 0;
        // initial multiplication pass with carry counting
        for (int i = 0; i < n + m - 1; ++i) {
            uint32_t sum = 0;
            for (int j = 0; j <= i; ++j)
                if (j < n && (i - j) < m)
                    sum += lookupMul[A[j]][B[i - j]];
            vedicAdd(res, i, sum);
        }
        int mulCount = n * m;
        int firstCarryAdds = carryCount;

        uint64_t totalMulCycles = 0, totalAddCycles = 0, bestOpCycles = UINT64_MAX;
        // timed trials
        for (int t = 0; t < TRIALS; ++t) {
            carryCount = 0;
            fill(res.begin(), res.end(), 0);

            _mm_lfence();
            uint64_t mulStart = rdtsc();
            for (int i = 0; i < n + m - 1; ++i) {
                uint32_t sum = 0;
                for (int j = 0; j <= i; ++j)
                    if (j < n && (i - j) < m)
                        sum += lookupMul[A[j]][B[i - j]];
                vedicAdd(res, i, sum);
            }
            _mm_lfence();
            uint64_t mulEnd = rdtsc();

            _mm_lfence();
            uint64_t addStart = rdtsc();
            for (int i = 0; i + 1 < (int)res.size(); ++i)
                if (res[i] >= 10) vedicAdd(res, i, 0);
            _mm_lfence();
            uint64_t addEnd = rdtsc();

            uint64_t ops = (mulEnd - mulStart) + (addEnd - addStart);
            totalMulCycles += (mulEnd - mulStart);
            totalAddCycles += (addEnd - addStart);
            bestOpCycles = min(bestOpCycles, ops);
        }

        double avgMul = double(totalMulCycles) / TRIALS;
        double avgAdd = double(totalAddCycles) / TRIALS;
        double avgTotal = avgMul + avgAdd;

        // final product formatting
        vector<uint16_t> finalRes(n + m);
        for (int i = 0; i < n + m - 1; ++i) {
            uint32_t sum = 0;
            for (int j = 0; j <= i; ++j)
                if (j < n && (i - j) < m) sum += lookupMul[A[j]][B[i - j]];
            vedicAdd(finalRes, i, sum);
        }
        for (int i = 0; i + 1 < (int)finalRes.size(); ++i)
            if (finalRes[i] >= 10) vedicAdd(finalRes, i, 0);
        while (finalRes.size() > 1 && finalRes.back() == 0) finalRes.pop_back();

        string product;
        for (auto it = finalRes.rbegin(); it != finalRes.rend(); ++it)
            product += char('0' + *it);

        int baseAddCount = mulCount - 2;
        int carryAddCount = firstCarryAdds;

        results.push_back({s1, s2, product,
                           mulCount, baseAddCount, carryAddCount,
                           avgTotal, avgMul, avgAdd, bestOpCycles});

        // print round summary
        cout << "\n=== Round " << round << " ===\n";
        cout << "Inputs       : " << s1 << " × " << s2 << "\n";
        cout << "Product      : " << product << "\n";
        cout << "Mul count    : " << mulCount << "\n";
        cout << "Add count    : " << baseAddCount << "\n";
        cout << "Carry Adds   : " << carryAddCount << "\n";
        cout << "Avg mul cycles  : " << avgMul << "\n";
        cout << "Avg add cycles  : " << avgAdd << "\n";
        cout << "Avg total cycles: " << avgTotal << "\n";
        cout << "Best total op cycles: " << bestOpCycles << "\n\n";
    }

    // write summary to file
    ofstream fout("results.txt");
    for (size_t i = 0; i < results.size(); ++i) {
        auto &r = results[i];
        fout << "Round " << (i+1) << ":\n"
             << "  Inputs : " << r.a << ", " << r.b << "\n"
             << "  Product: " << r.product << "\n"
             << "  Mults  : " << r.mulCount << "\n"
             << "  Adds   : " << r.baseAddCount << "\n"
             << "  Carries: " << r.carryAddCount << "\n"
             << "  AvgMulCycles : " << r.avgMulCycles << "\n"
             << "  AvgAddCycles : " << r.avgAddCycles << "\n"
             << "  AvgTotalCycles: " << r.avgCycles   << "\n"
             << "  BestOpCycles  : " << r.bestTotalOpCycles  << "\n\n";
    }
    cout << "Summary written to results.txt\n";
    return 0;
}
