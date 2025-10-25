/**
 * @file DifferentialVTCodes.hpp
 * @brief Declares the function for generating Differential VT-Codes (D-VT).
 */

#ifndef DIFFERENTIALVTCODES_HPP_
#define DIFFERENTIALVTCODES_HPP_

#include <string>
#include <vector>

/**
 * @brief Generates all D-VT codewords of length n with syndrome s.
 *
 * This function implements the exhaustive search for q-ary Differential
 * Varshamov-Tenengolts (D-VT) codes.
 *
 * A word x is a D-VT codeword if:
 * Sum_{i=1 to n} (i * y_i) ≡ s (mod n*q)
 *
 * Where q=4, and y is the differential vector:
 * y_i = (x_i - x_{i+1}) mod q  (for i < n)
 * y_n = x_n
 *
 * @param n The length of the codewords (codeLen).
 * @param s The target syndrome parameter.
 * @param num_threads The number of threads to use for generation.
 * @return A vector of strings representing the D-VT codewords.
 */
std::vector<std::string> GenerateDifferentialVTCodes(int n, int s, unsigned int num_threads);

#endif /* DIFFERENTIALVTCODES_HPP_ */