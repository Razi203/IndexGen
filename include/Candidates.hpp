/**
 * @file Candidates.hpp
 * @brief Declares functions for generating and filtering candidate codewords.
 *
 * This module is responsible for creating the initial pool of candidate strings that will be
 * used to construct the final codebook. It supports generating candidates using either
 * exhaustive enumeration or linear block codes for an initial Hamming distance guarantee.
 * It also provides functions to filter these candidates based on common biological
 * constraints, such as GC-content and maximum homopolymer run length.
 */

#ifndef CANDIDATES_HPP_
#define CANDIDATES_HPP_

#include "IndexGen.hpp"
#include <vector>
#include <string>

using namespace std;

/**
 * @brief The main function to generate a filtered list of candidate strings.
 *
 * This function orchestrates the candidate generation process based on the provided parameters.
 * It first generates an initial set of strings (either all possible strings or a linear code)
 * and then applies a series of filters (GC-content, max run length) to produce the final
 * set of candidates for codebook construction.
 *
 * @param params A struct containing all the necessary parameters, including code length,
 * minimum Hamming distance, GC-content bounds, and max run length.
 * @return A vector of strings representing the filtered candidates.
 */
vector<string> Candidates(const Params &params);

/**
 * @brief A test function to verify the properties of a generated linear code.
 *
 * @param n The length of the codewords to generate.
 * @param d The minimum Hamming distance the code should have.
 */
void TestCandidates(const int n, const int d);

#endif /* CANDIDATES_HPP_ */
