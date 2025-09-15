/**
 * @file Decode.hpp
 * @brief Declares functions for decoding received words using a provided codebook.
 *
 * This module implements the decoding process for error correction. Given a codebook
 * (a set of valid codewords) and a list of received words that may contain errors,
 * it finds the most likely original codeword for each received word. This is achieved
 * by finding the codeword with the minimum Levenshtein (edit) distance to the
 * received word, a process known as nearest-neighbor decoding. The implementation
 * supports multi-threading to accelerate the decoding of large datasets.
 */

#ifndef DECODE_HPP_
#define DECODE_HPP_

#include <vector>
#include <string>

using namespace std;

/**
 * @brief Decodes a vector of received words to their nearest neighbors in the codebook.
 *
 * This function iterates through each word in `receivedWords`, finds the closest codeword
 * from the `codebook` based on edit distance, and stores the result in `decodedWords`.
 * The process can be parallelized across multiple CPU cores by setting `threadNum`.
 *
 * @param codebook A vector of valid codewords.
 * @param receivedWords A vector of words to be decoded, which may contain errors.
 * @param decodedWords An output vector where the resulting decoded words will be stored.
 * @param codeLen The length of the codewords (assumed to be uniform).
 * @param threadNum The number of threads to use for parallel execution.
 */
void Decode(const vector<string> &codebook, const vector<string> &receivedWords, vector<string> &decodedWords,
			const int codeLen, const int threadNum);

/**
 * @brief A test function to verify the correctness and measure the performance of the decoding process.
 *
 * It loads a codebook from a file, generates a number of random words, decodes them,
 * and verifies that the decoding results are correct by comparing them to a brute-force check.
 *
 * @param testNum The number of test iterations to run.
 * @param wordNum The number of random words to generate and decode per test iteration.
 * @param codebookFilename The path to the file containing the codebook.
 * @param threadNum The number of threads to use for decoding.
 */
void TestDecode(const int testNum, const int wordNum, const string &codebookFilename, const int threadNum);

#endif /* DECODE_HPP_ */
