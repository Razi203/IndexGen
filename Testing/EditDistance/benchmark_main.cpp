#include <chrono>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

// Include your existing CPU implementation
#include "EditDistance.hpp"
// Include the new GPU header
#include "EditDistanceGPU.cuh"

void GenerateData(int num, int len, std::string &query, std::vector<char> &candidates)
{
    std::cout << "Generating " << num << " candidates of length " << len << "..." << std::endl;
    const char bases[] = "ACGT";
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 3);

    // Generate Query
    query.resize(len);
    for (int i = 0; i < len; ++i)
        query[i] = bases[dist(rng)];

    // Generate Candidates (Flat)
    candidates.resize(num * len);
    for (size_t i = 0; i < candidates.size(); ++i)
    {
        candidates[i] = bases[dist(rng)];
    }
}

int main()
{
    // Configuration
    const int NUM_CANDIDATES = 1000000; // 1 Million
    const int LEN = 64;                 // Must be <= 64 for this kernel

    std::string query;
    std::vector<char> candidates;
    GenerateData(NUM_CANDIDATES, LEN, query, candidates);

    // -------------------- CPU BENCHMARK --------------------
    std::cout << "\nRunning CPU Benchmark (" << NUM_CANDIDATES << " items)..." << std::endl;
    std::vector<int> cpu_results(NUM_CANDIDATES);

    auto start_cpu = std::chrono::high_resolution_clock::now();

    // Use your EditDistance.hpp PatternHandle for fair comparison (precomputed)
    PatternHandle H(query);

    // Note: We reconstruct the string_view logic manually to interface with your HPP
    // In a real app, you might adapt HPP to take char* to avoid std::string construction cost here
    for (int i = 0; i < NUM_CANDIDATES; ++i)
    {
        // Construct temp string wrapper (zero-copy if using C++17 string_view,
        // but std::string ctor copies. For benchmark, we accept this copy overhead
        // or you can modify EditDistance.hpp to accept const char* and length)
        std::string s(&candidates[i * LEN], LEN);
        cpu_results[i] = EditDistanceExact(s, H);
    }

    auto end_cpu = std::chrono::high_resolution_clock::now();
    double cpu_ms = std::chrono::duration<double, std::milli>(end_cpu - start_cpu).count();
    std::cout << "CPU Time: " << cpu_ms << " ms" << std::endl;

    // -------------------- GPU BENCHMARK --------------------
    std::cout << "\nRunning GPU Benchmark (" << NUM_CANDIDATES << " items)..." << std::endl;
    std::vector<int> gpu_results(NUM_CANDIDATES);

    // Warmup
    ComputeEditDistanceBatchGPU(query, candidates, 1000, LEN, gpu_results.data());

    // Run
    float gpu_ms = ComputeEditDistanceBatchGPU(query, candidates, NUM_CANDIDATES, LEN, gpu_results.data());
    std::cout << "GPU Time: " << gpu_ms << " ms" << std::endl;
    std::cout << "Speedup:  " << (cpu_ms / gpu_ms) << "x" << std::endl;

    // -------------------- VALIDATION --------------------
    std::cout << "\nValidating..." << std::endl;
    int mismatches = 0;
    for (int i = 0; i < NUM_CANDIDATES; ++i)
    {
        if (cpu_results[i] != gpu_results[i])
        {
            mismatches++;
            if (mismatches < 5)
            {
                std::cout << "Mismatch at " << i << ": CPU=" << cpu_results[i] << " GPU=" << gpu_results[i]
                          << std::endl;
            }
        }
    }

    if (mismatches == 0)
    {
        std::cout << "✅ SUCCESS: Results match perfectly." << std::endl;
    }
    else
    {
        std::cout << "❌ FAILURE: " << mismatches << " mismatches found." << std::endl;
    }

    return 0;
}