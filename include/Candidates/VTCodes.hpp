#ifndef VTCODE_HPP
#define VTCODE_HPP

#include <vector>
#include <string>

/**
 * @brief Finds all 4-ary codewords of a given length satisfying specific conditions.
 *
 * This function searches for all words x = (x_1, ..., x_n) where each x_i is in {0, 1, 2, 3}
 * that satisfy the following two congruency relations:
 * 1. sum_{i=2 to n} (i-1)*alpha_i ≡ a (mod n)
 * where alpha_i = 1 if x_i >= x_{i-1}, and 0 otherwise.
 * 2. sum_{j=1 to n} x_j ≡ b (mod 4)
 *
 * The search is parallelized using threads.
 *
 * @param n The length of the codewords. Must be >= 1.
 * @param a The integer parameter for the first condition's modulo.
 * @param b The integer parameter for the second condition's modulo.
 * @param num_threads The number of threads to use for the search. If 0, it will
 * attempt to use the hardware concurrency level.
 * @return A vector of strings, where each string is a valid codeword.
 */
std::vector<std::string> GenerateVTCodes(int n, int a, int b, unsigned int num_threads);

#endif // VTCODE_HPP
