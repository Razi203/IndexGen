/**
 * @file WaveGen.hpp
 * @brief Declares the functions for the Progressive Wave Construction method for candidate generation.
 */

#ifndef WAVEGEN_HPP_
#define WAVEGEN_HPP_

#include "IndexGen.hpp" // For Params struct
#include <string>
#include <vector>

/**
 * @brief Generates candidate DNA sequences using the Progressive Wave Construction method.
 *
 * This method starts with a set of diverse seed sequences and then greedily selects
 * candidates from a large pool of random strings, ensuring the minimum distance
 * constraint is met at each step.
 *
 * @param params The main configuration object containing all parameters.
 * @return A std::vector<std::string> of candidate DNA sequences.
 */
std::vector<std::string> GenProgressiveWave(const Params &params);

#endif /* WAVEGEN_HPP_ */
