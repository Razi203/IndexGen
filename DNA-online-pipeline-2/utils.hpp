#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <climits>
#include <random>
#include <cstdint>

using namespace std;

/**
 * Compute Hamming distance between two strings of equal length.
 *
 * @param s1 First string
 * @param s2 Second string
 * @return Hamming distance between the strings, or max int value if lengths differ
 */
int hamming_distance(const std::string &s1, const std::string &s2);

/**
 * Compute edit distance (edit distance) between two strings.
 *
 * @param s1 First string
 * @param s2 Second string
 * @return Minimum number of single-character edits (insertions, deletions, substitutions)
 */
int edit_distance(const std::string &s1, const std::string &s2, int limit = INT_MAX);

int FastEditDistance(const std::string &source, const std::string &target);

int FastEditDistanceWithThresholdBanded(const string &source, const string &target, int threshold);

class RandomStreamGenerator
{
private:
    std::mt19937_64 rng;

public:
    explicit RandomStreamGenerator(uint64_t seed);

    std::vector<uint8_t> getNextBytes(size_t length);

    void reset(uint64_t seed);
};

// Helper function to convert a single nucleotide to 2 bits
uint8_t nucleotide_to_2bit(char nucleotide);

// Helper function to convert 2 bits to nucleotide
char two_bit_to_nucleotide(uint8_t bits);

// Helper function to convert ACGT string to 2-bit encoded string
std::string acgt_to_2bit(const std::string &acgt_read);

// Helper function to convert 2-bit encoded string back to ACGT string
std::string two_bit_to_acgt(const std::string &encoded_read, size_t original_length);

int MyersEditDistance(const string &a, const string &b);
int MyersSingleWord(const string &p, const string &t);
int MyersMultiWord(const string &p, const string &t);
#endif // UTILS_HPP