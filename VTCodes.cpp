/**
 * @file VTCodes.cpp
 * @brief Implements the non-binary Varshamov-Tenengolts (VT) code generator.
 */

#include "VTCodes.hpp"
#include <vector>
#include <string>
#include <optional>
#include <iostream>

// Use a nested helper function within an anonymous namespace to hide implementation details.
namespace
{

    /**
     * @brief A recursive helper function to generate VT code prefixes with memoization.
     *
     * This is the core of the generator. It builds codewords from left to right (index 1 to n).
     *
     * @param index The current position in the string to build (from 1 to n).
     * @param current_sum The VT-sum accumulated so far for the prefix of length (index - 1).
     * @param n The total length of the final codewords.
     * @param a The target congruence value.
     * @param memo A memoization table to store results of subproblems, avoiding re-computation.
     * memo[index][sum] stores all valid suffixes starting from 'index' that, when
     * added to a prefix with 'sum', will result in a valid codeword.
     * @return A vector of valid suffixes for the given state.
     */
    const std::vector<std::string> &generate_recursive(
        int index,
        int current_sum,
        int n,
        int a,
        std::vector<std::vector<std::optional<std::vector<std::string>>>> &memo)
    {

        // Base Case: If we have constructed a full-length string
        if (index > n)
        {
            // Check if the total accumulated sum satisfies the VT condition
            if (current_sum % (n + 1) == a)
            {
                // Return a vector with an empty string, signifying a successful path.
                static const std::vector<std::string> success_sentinel = {""};
                return success_sentinel;
            }
            else
            {
                // Return an empty vector for an unsuccessful path.
                static const std::vector<std::string> failure_sentinel = {};
                return failure_sentinel;
            }
        }

        // Memoization Check: If we've already computed this subproblem, return the stored result.
        if (memo[index][current_sum])
        {
            return *memo[index][current_sum];
        }

        // --- Recursive Step ---
        std::vector<std::string> results;
        // Iterate through each possible character for the current position ('A', 'C', 'G', 'T')
        for (int char_val = 0; char_val < 4; ++char_val)
        {
            // Calculate the new running VT-sum
            int new_sum = (current_sum + index * char_val) % (n + 1);

            // Recursively find all valid suffixes that can follow the current character
            const std::vector<std::string> &suffixes = generate_recursive(index + 1, new_sum, n, a, memo);

            // Combine the current character with each valid suffix
            for (const std::string &suffix : suffixes)
            {
                results.push_back(std::to_string(char_val) + suffix);
            }
        }

        // Store the result in the memoization table and return it.
        memo[index][current_sum] = std::move(results);
        return *memo[index][current_sum];
    }

} // anonymous namespace

// See VTCodes.hpp for documentation.
std::vector<std::string> GenVTCodeStrings(int n, int a)
{
    if (n <= 0)
    {
        return {};
    }

    // The memoization table stores results for each state (index, current_sum).
    // index runs from 1 to n+1. current_sum runs from 0 to n.
    // Using std::optional to distinguish between "not computed" and "computed, result is empty".
    std::vector<std::vector<std::optional<std::vector<std::string>>>> memo(
        n + 2,
        std::vector<std::optional<std::vector<std::string>>>(n + 1));

    std::cout << "Generating non-binary VT code for n=" << n << ", a=" << a << "..." << std::endl;
    // Start the recursion at index 1 with an initial sum of 0.
    return generate_recursive(1, 0, n, a, memo);
}