#ifndef CUSTOM2_HPP
#define CUSTOM2_HPP

#include <vector>
#include <string>

/**
 * @brief Finds all 4-ary codewords of a given length satisfying a specific condition.
 *
 * This function searches for all words x = (x_1, ..., x_n) where each x_i is in {0, 1, 2, 3}
 * that satisfy the following congruency relation:
 * 1. sum_{i=1 to n} (i * x_i) ≡ a (mod n + 1)
 *
 * The search is parallelized using threads.
 *
 * @param n The length of the codewords. Must be a non-negative integer.
 * @param a The integer parameter for the condition's modulo.
 * @param num_threads The number of threads to use for the search. If 0, it will
 * attempt to use the hardware concurrency level.
 * @return A vector of strings, where each string is a valid codeword.
 */
std::vector<std::string> GenerateCustomCodes2(int n, int a, unsigned int num_threads);

#endif // CUSTOM2_HPP
