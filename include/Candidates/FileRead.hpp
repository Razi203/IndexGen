/**
 * @file FileRead.hpp
 * @brief Provides logic for reading and parsing candidate vectors from a file.
 */

#ifndef FILEREAD_HPP_
#define FILEREAD_HPP_

#include <string>
#include <vector>

/**
 * @brief Reads candidate vectors from a file.
 *
 * This function handles opening the file, detecting headers/separators,
 * parsing vectors (0123 or ACGT), and filtering by length.
 *
 * @param filename The path to the file containing candidates.
 * @param codeLen The required length of the candidate vectors.
 * @return A vector of strings containing the valid candidate vectors.
 * @throws std::runtime_error if the file cannot be opened.
 */
std::vector<std::string> ReadFileCandidates(const std::string &filename, int codeLen);

#endif /* FILEREAD_HPP_ */
