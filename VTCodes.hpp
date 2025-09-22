/**
 * @file VTCodes.hpp
 * @brief Declares the function for generating non-binary Varshamov-Tenengolts (VT) codes.
 *
 * These codes consist of vectors (x_1, ..., x_n) over an alphabet of size q
 * where the weighted sum (the "VT-sum") is congruent to a specific value 'a'
 * modulo (n+1).
 * i.e., sum_{i=1 to n} (i * x_i) = a (mod n+1)
 */

#ifndef VTCODES_HPP_
#define VTCODES_HPP_

#include <vector>
#include <string>

/**
 * @brief Generates a non-binary Varshamov-Tenengolts (VT) code.
 *
 * This function produces all strings of a given length over the alphabet {0, 1, 2, 3}
 * that satisfy the VT-code congruence condition. The implementation uses a recursive
 * approach with memoization for efficiency.
 *
 * @param n The length of the codewords.
 * @param a The congruence value for the VT-sum modulo (n+1). Defaults to 0.
 * @return A vector of strings representing all codewords in the VT code VT_a^4(n).
 */
std::vector<std::string> GenVTCodeStrings(int n, int a = 0);

#endif /* VTCODES_HPP_ */