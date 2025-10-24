/**
 * @file GenMat.hpp
 * @brief Declares functions that provide pre-computed generator matrices for various linear codes over GF(4).
 *
 * This header file provides access to specific, hard-coded generator matrices. These
 * matrices are essential for the encoding process in the `LinearCodes` module. Each function
 * corresponds to a well-known linear code with specific parameters [n, k, d], where 'n' is
 * the codeword length, 'k' is the message length, and 'd' is the minimum Hamming distance.
 */

#ifndef GENMAT_HPP_
#define GENMAT_HPP_

#include <vector>

// Using `std` for convenience.
using namespace std;

/**
 * @brief Returns the generator matrix for a [21, 18, 3] code over GF(4).
 * @return A 2D vector representing the 18x21 generator matrix.
 */
vector<vector<int>> Gen_21_18_3();

/**
 * @brief Returns the generator matrix for a [41, 36, 4] code over GF(4).
 * @return A 2D vector representing the 36x41 generator matrix.
 */
vector<vector<int>> Gen_41_36_4();

/**
 * @brief Returns the generator matrix for a [43, 36, 5] cyclic code over GF(4).
 * @return A 2D vector representing the 36x43 generator matrix.
 */
vector<vector<int>> Gen_43_36_5();

#endif /* GENMAT_HPP_ */
