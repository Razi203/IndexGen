
#include "pipeline_utils.hpp"
#include <string>
#include <vector>
#include <limits>
#include <climits>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <random>
#include <cstdint>
#include <cassert>
#include <array>

#include <cstdint>

namespace indexgen {
namespace clustering {
namespace impl {

using namespace std;

int hamming_distance(const std::string &s1, const std::string &s2)
{
    // Check if strings have equal length
    if (s1.length() != s2.length())
    {
        return std::numeric_limits<int>::max(); // equivalent to float('inf')
    }

    int distance = 0;
    for (size_t i = 0; i < s1.length(); ++i)
    {
        if (s1[i] != s2[i])
        {
            distance++;
        }
    }

    return distance;
}

int edit_distance(const std::string &s1, const std::string &s2, int limit)
{
    const size_t len1 = s1.length();
    const size_t len2 = s2.length();

    // Handle edge cases
    if (len1 == 0)
        return static_cast<int>(len2) > limit ? INT_MAX : static_cast<int>(len2);
    if (len2 == 0)
        return static_cast<int>(len1) > limit ? INT_MAX : static_cast<int>(len1);

    // Early termination if length difference exceeds limit
    if (abs((int)len1 - (int)len2) > limit)
        return INT_MAX;

    // Create a 2D vector to store distances
    std::vector<std::vector<int>> dp(len1 + 1, std::vector<int>(len2 + 1));

    // Initialize first row and column
    for (size_t i = 0; i <= len1; ++i)
    {
        dp[i][0] = i;
    }
    for (size_t j = 0; j <= len2; ++j)
    {
        dp[0][j] = j;
    }

    // Fill the dp table
    for (size_t i = 1; i <= len1; ++i)
    {
        bool row_has_valid = false;
        for (size_t j = 1; j <= len2; ++j)
        {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({
                dp[i - 1][j] + 1,       // deletion
                dp[i][j - 1] + 1,       // insertion
                dp[i - 1][j - 1] + cost // substitution
            });

            // Check if this cell is within the limit
            if (dp[i][j] <= limit)
                row_has_valid = true;
        }

        // If no cell in this row is within limit, we can terminate early
        if (!row_has_valid)
            return INT_MAX;
    }

    return dp[len1][len2] > limit ? INT_MAX : dp[len1][len2];
}

int FastEditDistanceWithThresholdBanded(const string &source, const string &target, int threshold)
{
    if (source.size() > target.size())
    {
        return FastEditDistanceWithThresholdBanded(target, source, threshold);
    }

    const int min_size = source.size(), max_size = target.size();

    // Early termination: if size difference exceeds threshold
    if (max_size - min_size > threshold)
    {
        return threshold + 1;
    }

    vector<int> lev_dist(min_size + 1);

    // Initialize first row
    for (int i = 0; i <= min_size; ++i)
    {
        lev_dist[i] = i;
    }

    for (int j = 1; j <= max_size; ++j)
    {
        int previous_diagonal = lev_dist[0], previous_diagonal_save;
        ++lev_dist[0];

        // Banded optimization: only compute cells within threshold band
        int start_i = max(1, j - threshold);
        int end_i = min(min_size, j + threshold);

        // Track if any cell in the band is within threshold
        bool within_threshold = (lev_dist[0] <= threshold);

        for (int i = start_i; i <= end_i; ++i)
        {
            previous_diagonal_save = lev_dist[i];
            if (source[i - 1] == target[j - 1])
            {
                lev_dist[i] = previous_diagonal;
            }
            else
            {
                lev_dist[i] = min(min(lev_dist[i - 1], lev_dist[i]), previous_diagonal) + 1;
            }
            previous_diagonal = previous_diagonal_save;

            if (lev_dist[i] <= threshold)
            {
                within_threshold = true;
            }
        }

        // Early termination: if no cell in the band is within threshold
        if (!within_threshold)
        {
            return threshold + 1;
        }
    }

    return lev_dist[min_size] <= threshold ? lev_dist[min_size] : threshold + 1;
}

int FastEditDistance(const string &source, const string &target)
{
    return MyersEditDistance(source, target);
    // if (source.size() > target.size())
    // {
    //     return FastEditDistance(target, source);
    // }

    // const int min_size = source.size(), max_size = target.size();
    // vector<int> lev_dist(min_size + 1);

    // for (int i = 0; i <= min_size; ++i)
    // {
    //     lev_dist[i] = i;
    // }

    // for (int j = 1; j <= max_size; ++j)
    // {
    //     int previous_diagonal = lev_dist[0], previous_diagonal_save;
    //     ++lev_dist[0];

    //     for (int i = 1; i <= min_size; ++i)
    //     {
    //         previous_diagonal_save = lev_dist[i];
    //         if (source[i - 1] == target[j - 1])
    //         {
    //             lev_dist[i] = previous_diagonal;
    //         }
    //         else
    //         {
    //             lev_dist[i] = min(min(lev_dist[i - 1], lev_dist[i]), previous_diagonal) + 1;
    //         }
    //         previous_diagonal = previous_diagonal_save;
    //     }
    // }

    // return lev_dist[min_size];
}

// length of p MUST BE <=64!
int MyersSingleWord(const string &p, const string &t)
{
    const size_t m = p.size();
    if (m == 0)
        return (int)t.size();
    assert(m <= 64);

    // Build Peq: map char -> bitmask (LSB = p[0])
    array<uint64_t, 256> Peq;
    Peq.fill(0);
    for (size_t i = 0; i < m; ++i)
    {
        unsigned char c = (unsigned char)p[i];
        Peq[c] |= (1ULL << i);
    }

    uint64_t PV = ~0ULL; // all ones
    uint64_t MV = 0ULL;
    int score = m;
    const uint64_t highest = 1ULL << (m - 1);

    for (char tc : t)
    {
        uint64_t Eq = Peq[(unsigned char)tc];
        // Step from Myers' paper
        uint64_t X = Eq | MV;
        uint64_t D0 = ((((X & PV) + PV) ^ PV) | X);
        uint64_t HN = PV & D0;
        uint64_t HP = MV | ~(PV | D0);

        // next PV, MV
        uint64_t X2 = (HP << 1) | 1ULL;
        MV = X2 & D0;
        PV = (HN << 1) | ~(X2 | D0);

        // adjust score by inspecting high bit
        if (HP & highest)
            ++score;
        else if (HN & highest)
            --score;
        // score += (HP >> (m - 1)) & 1;
        // score -= (HN >> (m - 1)) & 1;
    }
    return score;
}

int MyersMultiWord(const string &p, const string &t)
{
    assert(p.size() <= t.size()); // will work even if p longer than t, but will work faster if len p <= len t
    const size_t W = 64;
    const size_t m = p.size();
    if (!m)
        return (int)t.size();

    const size_t B = (m + W - 1) / W, last = B - 1, r = m - last * W;
    const uint64_t lastMask = (r == W) ? ~0ULL : ((1ULL << r) - 1ULL);

    // flat Peq table: 256 * B
    vector<uint64_t> Peq(256 * B);
    for (size_t i = 0; i < m; ++i)
        Peq[(uint8_t)p[i] * B + i / W] |= 1ULL << (i % W);

    vector<uint64_t> PV(B, ~0ULL), MV(B);
    PV[last] &= lastMask;

    int score = (int)m;
    const uint64_t highBit = 1ULL << ((m - 1) % W);

    for (uint8_t c : t)
    {
        uint64_t carry = 0, c1 = 1, c2 = 0, lastHP = 0, lastHN = 0;
        const uint64_t *EqBase = &Peq[c * B];

        for (size_t b = 0; b < B; ++b)
        {
            uint64_t Eq = EqBase[b], X = Eq | MV[b];

            // add with carry: (X & PV) + PV + carry
            uint64_t u = (X & PV[b]) + PV[b] + carry;
            carry = (u < PV[b]) || (carry && u == PV[b]);

            uint64_t D0 = (u ^ PV[b]) | X;
            uint64_t HN = PV[b] & D0;
            uint64_t HP = MV[b] | ~(PV[b] | D0);
            lastHP = HP;
            lastHN = HN;

            uint64_t X2 = (HP << 1) | c1;
            c1 = HP >> 63;
            uint64_t HNs = (HN << 1) | c2;
            c2 = HN >> 63;

            MV[b] = X2 & D0;
            PV[b] = HNs | ~(X2 | D0);
        }
        MV[last] &= lastMask;
        PV[last] &= lastMask;

        if (lastHP & highBit)
            ++score;
        else if (lastHN & highBit)
            --score;
    }
    return score;
}

int MyersEditDistance(const string &a, const string &b)
{
    const string &pattern = (a.size() <= b.size()) ? a : b;
    const string &text = (a.size() <= b.size()) ? b : a;
    if (pattern.size() <= 64)
        return MyersSingleWord(pattern, text);
    else
        return MyersMultiWord(pattern, text);
}



RandomStreamGenerator::RandomStreamGenerator(uint64_t seed) : rng(seed) {}

std::vector<uint8_t> RandomStreamGenerator::getNextBytes(size_t length)
{
    std::vector<uint8_t> result;
    result.reserve(length);

    for (size_t i = 0; i < length; ++i)
    {
        uint64_t randomValue = rng();
        result.push_back(static_cast<uint8_t>(randomValue & 0xFF));
    }

    return result;
}

void RandomStreamGenerator::reset(uint64_t seed)
{
    rng.seed(seed);
}

// Helper function to convert a single nucleotide to 2 bits
uint8_t nucleotide_to_2bit(char nucleotide)
{
    switch (nucleotide)
    {
    case 'A':
    case 'a':
        return 0b00; // A -> 00
    case 'C':
    case 'c':
        return 0b01; // C -> 01
    case 'G':
    case 'g':
        return 0b10; // G -> 10
    case 'T':
    case 't':
        return 0b11; // T -> 11
    default:
        throw std::invalid_argument("Invalid nucleotide: " + std::string(1, nucleotide));
    }
}

// Helper function to convert 2 bits to nucleotide
char two_bit_to_nucleotide(uint8_t bits)
{
    switch (bits & 0b11)
    { // Mask to ensure only 2 bits
    case 0b00:
        return 'A';
    case 0b01:
        return 'C';
    case 0b10:
        return 'G';
    case 0b11:
        return 'T';
    default:
        throw std::invalid_argument("Invalid 2-bit value");
    }
}

// Helper function to convert ACGT string to 2-bit encoded string
std::string acgt_to_2bit(const std::string &acgt_read)
{
    std::string encoded;

    // Process 4 nucleotides at a time (pack into 1 byte)
    for (size_t i = 0; i < acgt_read.length(); i += 4)
    {
        uint8_t packed_byte = 0;

        // Pack up to 4 nucleotides into one byte
        for (int j = 0; j < 4 && (i + j) < acgt_read.length(); ++j)
        {
            uint8_t nucleotide_bits = nucleotide_to_2bit(acgt_read[i + j]);
            packed_byte |= (nucleotide_bits << (6 - 2 * j)); // Shift to correct position
        }

        encoded.push_back(static_cast<char>(packed_byte));
    }

    return encoded;
}

// Helper function to convert 2-bit encoded string back to ACGT string
std::string two_bit_to_acgt(const std::string &encoded_read, size_t original_length)
{
    std::string acgt_read;
    acgt_read.reserve(original_length);

    size_t nucleotides_extracted = 0;

    for (size_t i = 0; i < encoded_read.length() && nucleotides_extracted < original_length; ++i)
    {
        uint8_t packed_byte = static_cast<uint8_t>(encoded_read[i]);

        // Extract up to 4 nucleotides from this byte
        for (int j = 0; j < 4 && nucleotides_extracted < original_length; ++j)
        {
            uint8_t nucleotide_bits = (packed_byte >> (6 - 2 * j)) & 0b11;
            acgt_read.push_back(two_bit_to_nucleotide(nucleotide_bits));
            nucleotides_extracted++;
        }
    }

    return acgt_read;
}

} // namespace impl
} // namespace clustering
} // namespace indexgen
