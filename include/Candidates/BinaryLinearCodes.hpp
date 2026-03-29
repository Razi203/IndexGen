/**
 * @file BinaryLinearCodes.hpp
 * @brief Provides functions for generating binary linear codes over GF(2) with a specified minimum Hamming distance.
 *
 * This module generates sets of binary vectors (codewords over {0,1}) that are guaranteed to be
 * separated by a minimum Hamming distance. It uses Hamming codes (d=2-4) and BCH codes (d=5-7).
 * The codes are constructed algorithmically rather than from pre-computed matrices.
 */

#ifndef BINARYLINEARCODES_HPP_
#define BINARYLINEARCODES_HPP_

#include <vector>
#include <string>

using namespace std;

/**
 * @brief Generates all possible binary codewords of a given length and minimum Hamming distance.
 *
 * Constructs a binary linear code over GF(2) with length n and minimum Hamming distance minHammDist.
 * Enumerates all 2^k possible data vectors and encodes them to produce the complete codebook.
 *
 * @param n The desired length of the output codewords.
 * @param minHammDist The desired minimum Hamming distance (2-7).
 * @param bias The bias vector to XOR with each codeword (length n, values in {0,1}).
 * @param row_perm The row permutation vector for the generator matrix.
 * @param col_perm The column permutation vector for the generator matrix.
 * @return A vector of vector<int>, where each inner vector is a binary codeword of length n
 *         with elements in {0, 1}.
 */
vector<vector<int>> BinaryCodedVecs(const int n, const int minHammDist, const vector<int> &bias,
                                     const vector<int> &row_perm, const vector<int> &col_perm);

/**
 * @brief Generates binary codewords with default parameters (zero bias, identity permutations).
 *
 * @param n The desired length of the output codewords.
 * @param minHammDist The desired minimum Hamming distance (2-7).
 * @return A vector of vector<int>, where each inner vector is a binary codeword.
 */
vector<vector<int>> BinaryCodedVecs(const int n, const int minHammDist);

#endif /* BINARYLINEARCODES_HPP_ */
