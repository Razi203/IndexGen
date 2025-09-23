#ifndef VTCODES_HPP
#define VTCODES_HPP

#include <vector>
#include <string>
#include <iostream>

/**
 * @brief Generates all 4-ary VT code words of length n with parameter a using a single thread and returns them in a vector.
 *
 * A q-ary word (x_1, ..., x_n) is a VT code word if sum(i * x_i) % (n+1) == a.
 * For this function, q = 4 and the alphabet is {0, 1, 2, 3}.
 *
 * @param n The length of the words. Must be a positive integer.
 * @param a The VT code parameter for the congruence relation, i.e., sum % (n+1) == a.
 * @return A vector of strings, each being a valid VT code word.
 * @note This is a simple, non-parallel implementation. For better performance on modern hardware,
 * consider using GenerateVTCodesMT_Mem.
 */
std::vector<std::string> GenerateVTCodes(int n, int a);

/**
 * @brief Generates all 4-ary VT code words of length n with parameter a using multiple threads and writes them to an output stream.
 *
 * This function is designed for performance and for values of n that are too large
 * to store the results in memory. It partitions the search space and processes it in parallel, streaming results directly.
 *
 * @param n The length of the words. Must be a positive integer.
 * @param a The VT code parameter for the congruence relation, i.e., sum % (n+1) == a.
 * @param out The output stream (e.g., std::cout or an std::ofstream) to write the generated words to. Each word is followed by a newline.
 * @param num_threads The number of threads to use. If 0, it defaults to the number of concurrent threads supported by the hardware.
 */
void GenerateVTCodesMT(int n, int a, std::ostream &out, unsigned int num_threads = 0);

/**
 * @brief Generates all 4-ary VT code words of length n with parameter a using multiple threads and returns them in a vector.
 *
 * This function is a multi-threaded version of GenerateVTCodes. It offers significantly
 * better performance but can consume a large amount of memory for large n.
 *
 * @param n The length of the words. Must be a positive integer.
 * @param a The VT code parameter for the congruence relation, i.e., sum % (n+1) == a.
 * @param num_threads The number of threads to use. If 0, it defaults to the number of concurrent threads supported by the hardware.
 * @return A vector of strings, each being a valid VT code word.
 */
std::vector<std::string> GenerateVTCodesMT_Mem(int n, int a, unsigned int num_threads = 0);

#endif // VTCODES_HPP
