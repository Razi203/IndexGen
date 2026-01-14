#ifndef DNAV8_amir_HPP
#define DNAV8_amir_HPP

#include <vector>
#include <string>

/**
 * @brief Reconstructs the original DNA sequence from copies using CPL implementation
 *
 * This function takes a vector of DNA sequence copies and reconstructs the most likely
 * original sequence using edit distance algorithms and clustering techniques.
 *
 * @param copies Vector of DNA sequence strings representing copies of the original sequence
 * @return std::string The reconstructed original DNA sequence
 */
std::string reconstruct_cpl_imp(std::vector<std::string> &copies, int original_length);

#endif // DNAV8_HPP