/**
 * @file Utils.hpp
 * @brief A collection of utility functions for bioinformatics, string manipulation, and coding theory.
 *
 * This file provides a wide range of helper functions, including:
 * - Levenshtein (Edit) and Hamming distance calculations.
 * - Tests for specific string properties like GC-content and character runs.
 * - Mathematical operations over Galois Field GF(4).
 * - Functions for serializing and deserializing data structures to/from files.
 * - Random string generation and content analysis for DNA-like sequences.
 */

#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <string>
#include <vector>
#include <atomic>
#include <random>
#include <chrono>
#include "IndexGen.hpp"

using namespace std;

// Forward declaration for the Params struct defined in IndexGen.hpp
struct Params;

// =================================================================================
// SECTION: STRING DISTANCE METRICS
// =================================================================================

/**
 * @brief Calculates the exact Levenshtein (edit) distance between two strings.
 * @details Implements a space-optimized version of the Wagner–Fischer algorithm.
 * The distance is the minimum number of single-character edits (insertions,
 * deletions, or substitutions) required to change one string into the other.
 * Time complexity: O(m*n), Space complexity: O(min(m, n)).
 * @param source The first string.
 * @param target The second string.
 * @return The Levenshtein distance between the two strings.
 */
int FastEditDistance(const string &source, const string &target);

/**
 * @brief Checks if the Levenshtein distance between two strings is at least a given value.
 * @details This is an optimized version that can terminate early if the distance is
 * guaranteed to exceed the threshold, making it faster than calculating the full distance.
 * @param source The first string.
 * @param target The second string.
 * @param minED The minimum edit distance threshold.
 * @return `true` if the edit distance is greater than or equal to `minED`, `false` otherwise.
 */
bool FastEditDistance(const string &source, const string &target, const int minED);

/**
 * @brief A specialized edit distance check for DNA-like sequences ('0123').
 * @details First, it performs a quick check based on the character counts (A, C, G, T).
 * If this preliminary check indicates the distance might be less than minED,
 * it proceeds with the full `FastEditDistance` calculation.
 * @param X The first string.
 * @param Y The second string.
 * @param minED The minimum edit distance threshold.
 * @param contx A vector containing the character counts ('0','1','2','3') for string X.
 * @param conty A vector containing the character counts for string Y.
 * @return `true` if the distance is likely >= `minED`, `false` otherwise.
 */
bool FastEditDistance0123(const string &X, const string &Y, const int minED, const vector<char> &contx,
						  const vector<char> &conty);

/**
 * @brief Calculates edit distance for searching, returning -1 if it exceeds a threshold.
 * @details Similar to the boolean check, but returns the actual distance if it's below
 * the threshold, or -1 if it's guaranteed to be equal to or greater than the threshold.
 * @param source The first string.
 * @param target The second string.
 * @param minED The minimum edit distance threshold.
 * @return The edit distance if it's less than `minED`, otherwise -1.
 */
int FastEditDistanceForSearch(const string &source, const string &target, const int minED);

/**
 * @brief A specialized version of `FastEditDistanceForSearch` for DNA-like sequences ('0123').
 * @details Combines the character count pre-check with the thresholded distance calculation.
 * @param X The first string.
 * @param Y The second string.
 * @param minED The minimum edit distance threshold.
 * @param contx A vector of character counts for string X.
 * @param conty A vector of character counts for string Y.
 * @return The edit distance if less than `minED`, otherwise -1.
 */
int FastEditDistance0123ForSearch(const string &X, const string &Y, const int minED, const vector<char> &contx,
								  const vector<char> &conty);

/**
 * @brief Calculates the Hamming distance between two strings of equal length.
 * @details The Hamming distance is the number of positions at which the corresponding
 * characters are different. Asserts that strings are of equal length.
 * @param str1 The first string.
 * @param str2 The second string.
 * @return The Hamming distance.
 */
int HammingDist(const string &str1, const string &str2);

/**
 * @brief Verifies that all pairs of strings in a vector have at least a minimum edit distance.
 * @details This function is multi-threaded to speed up the verification process.
 * @param vecs A vector of strings to check.
 * @param minED The minimum required edit distance between any two strings.
 * @param threadNum The number of threads to use for the computation.
 */
void VerifyDist(vector<string> &vecs, const int minED, const int threadNum);

/**
 * @brief Verifies that all pairs of strings in a vector have at least a minimum Hamming distance.
 * @param vecs A vector of strings to check.
 * @param minHammDist The minimum required Hamming distance.
 */
void VerifyHammDist(const vector<string> &vecs, const int minHammDist);

// =================================================================================
// SECTION: STRING PROPERTY CONSTRAINTS
// =================================================================================

/**
 * @brief Calculates the length of the longest run of identical consecutive characters in a string.
 * @param str The input string.
 * @return The length of the longest run. Example: "AAABBC" -> 3.
 */
int MaxRun(const string &str);

/**
 * @brief Tests if the GC-content of a DNA-like string falls within a given range.
 * @details Assumes '1' represents 'C' and '2' represents 'G'.
 * @param a The input string.
 * @param minGCCont The minimum acceptable GC-content (fraction, e.g., 0.4).
 * @param maxGCCont The maximum acceptable GC-content (fraction, e.g., 0.6).
 * @return `true` if the GC-content is within [`minGCCont`, `maxGCCont`], `false` otherwise.
 */
bool TestGCCont(const string &a, const double minGCCont, const double maxGCCont);

/**
 * @brief Checks if a string contains at least one of every character ('0','1','2','3').
 * @param a The input string.
 * @return `true` if all four characters are present, `false` otherwise.
 */
bool TestAllLettersOccurence(const string &a);

// =================================================================================
// SECTION: BASE-4 and GF(4) ARITHMETIC
// =================================================================================

/**
 * @brief Treats a vector of integers {0,1,2,3} as a base-4 number and calculates the next number.
 * @param vec The vector representing a base-4 number.
 * @return The vector representing the next base-4 number. Returns an empty vector on overflow.
 */
vector<int> NextBase4(const vector<int> &vec);

/**
 * @brief Performs matrix multiplication over the Galois Field GF(4).
 * @param v A row vector (1 x k).
 * @param M A matrix (k x l).
 * @param k The number of columns in v / rows in M.
 * @param l The number of columns in M.
 * @return The resulting row vector (1 x l).
 */
vector<int> MatMul(const vector<int> &v, const vector<vector<int>> &M, const int k, const int l);

/**
 * @brief Performs addition over the Galois Field GF(4).
 * @param a An integer in {0, 1, 2, 3}.
 * @param b An integer in {0, 1, 2, 3}.
 * @return The result of (a + b) in GF(4).
 */
int AddF4(const int a, const int b);

/**
 * @brief Performs multiplication over the Galois Field GF(4).
 * @details The field is constructed with the polynomial x^2 + x + 1.
 * @param a An integer in {0, 1, 2, 3}.
 * @param b An integer in {0, 1, 2, 3}.
 * @return The result of (a * b) in GF(4).
 */
int MulF4(const int a, const int b);

// =================================================================================
// SECTION: DATA TYPE CONVERSIONS & HELPERS
// =================================================================================

/**
 * @brief Converts a vector of integers {0,1,2,3} to its string representation.
 * @param vec The input vector of integers.
 * @return The corresponding string (e.g., {1,0,3} -> "103").
 */
string VecToStr(const vector<int> &vec);

/**
 * @brief Converts a GenerationMethod enum to a human-readable string.
 * @param method The enum value to convert.
 * @return A string representation of the method.
 */
std::string GenerationMethodToString(GenerationMethod method);

// =================================================================================
// SECTION: CONSOLE PRINTING
// =================================================================================

/**
 * @brief Prints the testing parameters to the console.
 * @param params A struct containing various parameters for the test run.
 */
void PrintTestParams(const Params &params);

/**
 * @brief Prints the results of a test run to the console.
 * @param candidateNum The number of candidate strings found.
 * @param matrixOnesNum The number of ones in a related matrix (context-specific).
 * @param codewordsNum The final number of codewords generated.
 */
void PrintTestResults(const int candidateNum, const long long int matrixOnesNum, const int codewordsNum);

// =================================================================================
// SECTION: FILE I/O OPERATIONS
// =================================================================================

/**
 * @brief Writes a vector of codewords and test parameters to a formatted output file.
 * @param codeWords The vector of strings to write.
 * @param params The parameters used to generate the codewords.
 * @param candidateNum The number of candidate strings.
 * @param matrixOnesNum The number of ones in the generation matrix.
 */
void ToFile(const vector<string> &codeWords, const Params &params, const int candidateNum,
			const long long int matrixOnesNum, const chrono::duration<double> &candidatesTime,
			const chrono::duration<double> &fillAdjListTime, const chrono::duration<double> &processMatrixTime,
			const chrono::duration<double> &overAllTime);

/**
 * @brief Serializes a `Params` struct to a file.
 * @param params The struct to save.
 * @param fileName The name of the output file.
 */
void ParamsToFile(const Params &params, const string &fileName);

/**
 * @brief Deserializes a `Params` struct from a file.
 * @param params The struct to load data into.
 * @param fileName The name of the input file.
 */
void FileToParams(Params &params, const string &fileName);

/**
 * @brief Writes a vector of integers to a file, one integer per line.
 * @param data The vector of integers.
 * @param fileName The name of the output file.
 */
void IntVecToFile(const vector<int> &data, const string &fileName);

/**
 * @brief Reads a vector of integers from a file.
 * @param data The vector to populate.
 * @param fileName The name of the input file.
 */
void FileToIntVec(vector<int> &data, const string &fileName);

/**
 * @brief Writes a vector of strings to a file, one string per line.
 * @param data The vector of strings.
 * @param fileName The name of the output file.
 */
void StrVecToFile(const vector<string> &data, const string &fileName);

/**
 * @brief Reads a vector of strings from a file.
 * @param data The vector to populate.
 * @param fileName The name of the input file.
 */
void FileToStrVec(vector<string> &data, const string &fileName);

/**
 * @brief Writes a single integer to a file.
 * @param num The integer to write.
 * @param fileName The name of the output file.
 */
void NumToFile(const int num, const string &fileName);

/**
 * @brief Reads a single integer from a file.
 * @param num The integer variable to populate.
 * @param fileName The name of the input file.
 */
void FileToNum(int &num, const string &fileName);

/**
 * @brief Writes a single long long integer to a file.
 * @param num The number to write.
 * @param fileName The name of the output file.
 */
void LongLongIntToFile(const long long int num, const string &fileName);

/**
 * @brief Reads a single long long integer from a file.
 * @param num The variable to populate.
 * @param fileName The name of the input file.
 */
void FileToLongLongInt(long long int &num, const string &fileName);

// =================================================================================
// SECTION: DNA-LIKE SEQUENCE ('0123') ANALYSIS
// =================================================================================

/**
 * @brief Calculates the sum of absolute differences between two character count vectors.
 * @param acgtContx A vector of character counts for the first string.
 * @param acgtConty A vector of character counts for the second string.
 * @return The sum of absolute differences.
 */
int SumAbs0123Diff(const vector<char> &acgtContx, const vector<char> &acgtConty);

/**
 * @brief Counts the occurrences of each character ('0','1','2','3') in a string.
 * @param s The input string.
 * @param cont0123 An output vector of size 4 to store the counts.
 */
void Cont0123(const string &s, vector<char> &cont0123);

/**
 * @brief Applies the `Cont0123` counting function to an entire vector of strings.
 * @param vec The vector of input strings.
 * @return A vector of vectors, where each inner vector contains the character counts
 * for the corresponding input string.
 */
vector<vector<char>> Cont0123(const vector<string> &vec);

/**
 * @brief Generates a random string of a given length over the alphabet {'0','1','2','3'}.
 * @param length The desired length of the string.
 * @param generator A reference to an mt19937 random number generator.
 * @return The generated random string.
 */
string MakeStrand0123(const unsigned length, mt19937 &generator);

#endif /* UTILS_HPP_ */
