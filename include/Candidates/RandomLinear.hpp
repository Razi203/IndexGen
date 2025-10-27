/**
 * @file RandomLinear.hpp
 * @brief Declares the function for generating random samples from linear codes.
 */

#ifndef RANDOMLINEAR_HPP_
#define RANDOMLINEAR_HPP_

#include <string>
#include <vector>

/**
 * @brief Generates N random candidates from a linear code.
 *
 * This function first generates all codewords using the LinearCodes module
 * with a specified minimum Hamming distance, then randomly selects N candidates
 * from the generated set.
 *
 * @param n The length of the codewords.
 * @param minHammDist The minimum Hamming distance for the linear code (2-5).
 * @param num_candidates The number of random candidates to select (N).
 * @param seed The random seed for reproducibility (0 for random seed).
 * @return A vector of strings representing the randomly selected codewords.
 */
std::vector<std::string> GenerateRandomLinearCandidates(int n, int minHammDist, int num_candidates,
                                                        unsigned int seed = 0);

#endif /* RANDOMLINEAR_HPP_ */