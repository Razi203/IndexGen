/**
 * @file MaxClique.hpp
 * @brief Defines functions to generate a codebook by solving the Maximum Clique problem.
 *
 * This module provides an alternative method for codebook generation. The workflow is as follows:
 * 1. A "compatibility graph" is constructed from the candidate strings. An edge exists
 * between two strings if their Levenshtein distance is *greater than or equal to*
 * the required minimum distance.
 * 2. The problem of finding the largest valid codebook is then equivalent to finding the
 * largest fully connected subgraph (a "maximum clique") in this compatibility graph.
 * 3. An external library (mcqd.h) is used to solve this NP-hard problem, guaranteeing
 * an optimal-sized codebook from the given candidates, though potentially at a high
 * computational cost.
 */

#ifndef MAXCLIQUE_HPP_
#define MAXCLIQUE_HPP_

#include <vector>
#include <string>
#include "IndexGen.hpp" // Assumed to contain the definition for Params struct

// Forward declaration for the Params struct defined in IndexGen.hpp
struct Params;

/**
 * @brief Main function to generate a codebook using the maximum clique algorithm.
 * @details This function orchestrates the entire process:
 * 1. Generates candidate strings based on the input parameters.
 * 2. Constructs an adjacency matrix representing the compatibility graph.
 * 3. Calls a solver to find the maximum clique.
 * 4. Converts the result back into a codebook of strings.
 * 5. Saves the final codebook and statistics.
 * @param params A struct containing all generation parameters (string length, min distance, etc.).
 */
void GenerateCodebookMaxClique(const Params &params);

#endif /* MAXCLIQUE_HPP_ */
