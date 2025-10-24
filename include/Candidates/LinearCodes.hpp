/**
 * @file LinearCodes.hpp
 * @brief Provides functions for generating linear block codes over GF(4) with a specified minimum Hamming distance.
 *
 * This module is used to create sets of vectors (codewords) that are guaranteed to be
 * separated by a minimum Hamming distance. This is a fundamental technique in error-correcting
 * codes and is used here to generate candidate DNA sequences that are robust against
 * substitution errors. The process involves using pre-defined generator matrices for
 * codes over the Galois Field of 4 elements (GF(4)), which naturally corresponds to the
 * four DNA bases.
 */

#ifndef LINEARCODES_HPP_
#define LINEARCODES_HPP_

#include <vector>
#include <string> // For std::string usage if needed, though not directly in this function signature.

// Using `std` namespace for convenience in this header.
// In larger projects, it's often better to use `std::` prefix.
using namespace std;

/**
 * @brief Generates all possible codewords of a given length and minimum Hamming distance.
 *
 * This is the main public function of the module. It constructs a linear code over GF(4)
 * with parameters `n` (length) and `k` (dimension), which is determined by the desired
 * minimum Hamming distance `minHammDist`. It then enumerates all `4^k` possible data vectors,
 * multiplies each by the generator matrix, and returns the complete set of `4^k` codewords.
 *
 * @param n The desired length of the output codewords.
 * @param minHammDist The desired minimum Hamming distance between any pair of codewords.
 * Supported values are 2, 3, 4, and 5.
 * @return A std::vector of std::vector<int>, where each inner vector is a codeword of length `n`
 * with elements in {0, 1, 2, 3}.
 */
vector<vector<int>> CodedVecs(const int n, const int minHammDist);

#endif /* LINEARCODES_HPP_ */
