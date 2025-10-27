/**
 * @file RandomLinear.cpp
 * @brief Implementation for generating random samples from linear codes.
 */

#include "Candidates/RandomLinear.hpp"
#include "Candidates/LinearCodes.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <chrono>
#include <random>

std::vector<std::string> GenerateRandomLinearCandidates(int n, int minHammDist, int num_candidates, unsigned int seed)
{
    // Step 1: Generate all linear code codewords
    std::vector<std::vector<int>> linearCodeVecs = CodedVecs(n, minHammDist);

    // Convert to strings
    std::vector<std::string> allCandidates;
    allCandidates.reserve(linearCodeVecs.size());
    for (const auto &vec : linearCodeVecs)
    {
        allCandidates.push_back(VecToStr(vec));
    }

    // If requesting more candidates than available, return all
    if (num_candidates >= static_cast<int>(allCandidates.size()))
    {
        return allCandidates;
    }

    // Step 2: Randomly select N candidates
    std::vector<std::string> selectedCandidates;
    selectedCandidates.reserve(num_candidates);

    // Initialize random number generator
    unsigned int actual_seed = seed;
    if (actual_seed == 0)
    {
        actual_seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }
    std::mt19937 rng(actual_seed);

    // Create indices and shuffle
    std::vector<int> indices(allCandidates.size());
    for (size_t i = 0; i < indices.size(); ++i)
    {
        indices[i] = i;
    }
    std::shuffle(indices.begin(), indices.end(), rng);

    // Select first N indices
    for (int i = 0; i < num_candidates; ++i)
    {
        selectedCandidates.push_back(allCandidates[indices[i]]);
    }

    return selectedCandidates;
}
