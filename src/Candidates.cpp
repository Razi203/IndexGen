/**
 * @file Candidates.cpp
 * @brief Implements the logic for generating and filtering candidate codewords.
 */

#include "Candidates.hpp"
#include "CandidateGenerator.hpp"
#include "Candidates/DifferentialVTCodes.hpp"
#include "Candidates/LinearCodes.hpp"
#include "Candidates/RandomLinear.hpp"
#include "Candidates/VTCodes.hpp"
#include "Utils.hpp"
#include <cassert>
#include <iostream>
#include <thread>

// --- Internal Generation and Filtering Functions ---

/**
 * @brief Generates all possible strings of length n over the alphabet {0, 1, 2, 3}.
 * @param n The desired length of the strings.
 * @return A vector containing all 4^n possible strings.
 */
vector<string> GenAllStrings(const int n)
{
    vector<string> result;
    vector<int> start(n, 0);
    // Iterate through all base-4 numbers of length n
    for (vector<int> vec = start; not vec.empty(); vec = NextBase4(vec))
    {
        result.push_back(VecToStr(vec));
    }
    return result;
}

vector<string> GenerateRandomCandidates(const int n, const int num_candidates, const int threadNum)
{
    vector<string> result;
    result.reserve(num_candidates);

    // Calculate candidates per thread
    int candidates_per_thread = num_candidates / threadNum;
    int remainder = num_candidates % threadNum;

    // Vector to store threads and their results
    vector<std::thread> threads;
    vector<vector<string>> thread_results(threadNum);

    // Lambda function for each thread to generate random strings
    auto generate_random = [n](int count, int seed, vector<string> &local_result)
    {
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> dist(0, 3);
        local_result.reserve(count);

        for (int i = 0; i < count; ++i)
        {
            string random_str;
            random_str.reserve(n);
            for (int j = 0; j < n; ++j)
            {
                random_str += std::to_string(dist(rng));
            }
            local_result.push_back(random_str);
        }
    };

    // Launch threads
    for (int i = 0; i < threadNum; ++i)
    {
        int count = candidates_per_thread + (i < remainder ? 1 : 0);
        int seed = std::random_device{}() + i;
        threads.emplace_back(generate_random, count, seed, std::ref(thread_results[i]));
    }

    // Wait for all threads to complete
    for (auto &thread : threads)
    {
        thread.join();
    }

    // Combine results from all threads
    for (const auto &local_result : thread_results)
    {
        result.insert(result.end(), local_result.begin(), local_result.end());
    }

    return result;
}

/**
 * @brief Generates all codewords of length n with a specified minimum Hamming distance.
 * @details This function uses the `LinearCodes` module to create a set of strings that
 * are guaranteed to be separated by at least `minHammDist`.
 * @param n The length of the codewords.
 * @param minHammDist The minimum Hamming distance (from 2 to 5).
 * @return A vector of strings representing the generated codewords.
 */
vector<string> GenAllCodeStrings(const int n, const int minHammDist)
{
    vector<vector<int>> codeVecs = CodedVecs(n, minHammDist);
    vector<string> codeWords;
    for (const vector<int> &vec : codeVecs)
    {
        codeWords.push_back(VecToStr(vec));
    }
    return codeWords;
}

/**
 * @brief A pass-through filter that does nothing.
 * @param strs The input strings.
 * @param filteredStrs The output vector, which will be a copy of the input.
 * @param params Unused.
 */
void NoFilter(const vector<string> &strs, vector<string> &filteredStrs, const Params &params)
{
    filteredStrs = strs;
}

/**
 * @brief Filters strings based on GC-content.
 * @details Keeps only the strings where the fraction of '1's (C) and '2's (G) is
 * within the specified range.
 * @param strs The input strings.
 * @param filteredStrs The output vector containing only strings that pass the filter.
 * @param params The parameter struct containing minGCCont and maxGCCont.
 */
void FilterGC(const vector<string> &strs, vector<string> &filteredStrs, const Params &params)
{
    for (const string &str : strs)
    {
        if (TestGCCont(str, params.minGCCont, params.maxGCCont))
        {
            filteredStrs.push_back(str);
        }
    }
}

/**
 * @brief Filters strings based on maximum homopolymer run length.
 * @details Keeps only the strings where the longest sequence of identical consecutive
 * characters is less than or equal to `params.maxRun`.
 * @param strs The input strings.
 * @param filteredStrs The output vector containing only strings that pass the filter.
 * @param params The parameter struct containing `maxRun`.
 */
void FilterMaxRun(const vector<string> &strs, vector<string> &filteredStrs, const Params &params)
{
    for (const string &str : strs)
    {
        if ((MaxRun(str) <= params.maxRun))
        {
            filteredStrs.push_back(str);
        }
    }
}

/**
 * @brief Filters strings based on both GC-content and maximum run length.
 * @param strs The input strings.
 * @param filteredStrs The output vector containing only strings that pass both filters.
 * @param params The parameter struct containing all filter criteria.
 */
void FilterGCMaxRun(const vector<string> &strs, vector<string> &filteredStrs, const Params &params)
{
    for (const string &str : strs)
    {
        if ((MaxRun(str) <= params.maxRun) && TestGCCont(str, params.minGCCont, params.maxGCCont))
        {
            filteredStrs.push_back(str);
        }
    }
}

// --- Public Function Implementations ---

// See Candidates.hpp for function documentation.
std::vector<std::string> Candidates(const Params &params)
{
    // Step 1: Create the appropriate generator using the factory function
    std::unique_ptr<CandidateGenerator> generator = CreateGenerator(params);

    // Step 2: Generate candidates
    std::vector<std::string> unfiltered = generator->generate();

    // Step 3: Apply filters and return
    return generator->applyFilters(unfiltered);
}

// See Candidates.hpp for function documentation.
void TestCandidates(const int n, const int d)
{
    vector<string> cand = GenAllCodeStrings(n, d);
    cout << "Testing code n=" << n << "\td=" << d << "\tcode size " << cand.size() << "...";

    // Verify that the Hamming distance is correct for all pairs.
    bool success = true;
    for (size_t i = 0; i < cand.size(); i++)
    {
        for (size_t j = i + 1; j < cand.size(); j++)
        {
            if (HammingDist(cand[i], cand[j]) < d)
            {
                success = false;
                break;
            }
        }
        if (!success)
            break;
    }

    if (!success)
    {
        cout << "FAILURE" << endl;
    }
    else
    {
        cout << "SUCCESS" << endl;
    }
}
