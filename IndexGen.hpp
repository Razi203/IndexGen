/**
 * @file IndexGen.hpp
 * @brief Defines the main parameter structure for the DNA codebook generator.
 *
 * This file contains the definition for the `Params` struct, which is used
 * throughout the application to configure the codebook generation and decoding
 * processes. It centralizes all user-configurable settings into a single object.
 */

#ifndef INDEXGEN_HPP_
#define INDEXGEN_HPP_

#include <memory>
#include <string>
#include <vector>

// Forward declarations for functions defined in other files
struct Params; // Forward declare Params struct for function prototypes

/**
 * @enum GenerationMethod
 * @brief Identifies the algorithm used for codebook generation.
 */
enum class GenerationMethod
{
    LINEAR_CODE,      ///< Uses linear codes to generate candidate strings with guaranteed Hamming distance.
    VT_CODE,          ///< Uses Varshamov-Tenengolts codes for candidate generation.
    ALL_STRINGS,      ///< Generates all possible strings of the specified length (brute-force).
    CUSTOM_1,         ///< Placeholder for a custom generation method 1.
    CUSTOM_2,         ///< Placeholder for a custom generation method 2.
    PROGRESSIVE_WAVE, ///< Generates candidates by expanding from diverse seed sequences.
    RANDOM            ///< Generates candidates using a randomization approach.
};

/**
 * @struct GenerationConstraints
 * @brief Base class for method-specific generation constraints.
 */
struct GenerationConstraints
{
    virtual ~GenerationConstraints() = default; // Ensures derived class destructors are called

    GenerationConstraints()
    {
    }
};

/**
 * @struct LinearCodeConstraints
 * @brief Constraints for the standard Hamming and Levenshtein distance-based generator.
 */
struct LinearCodeConstraints : public GenerationConstraints
{
    // --- Distance & Error Correction Parameters ---
    /**
     * @brief The minimum Hamming distance required for the initial set of candidate strings.
     * @details A value of 1 generates all possible strings. Values 2-5 use linear codes
     * to generate a smaller, pre-filtered set with a guaranteed Hamming distance,
     * which speeds up the overall process.
     * @note Valid values: {1, 2, 3, 4, 5}.
     */
    int candMinHD;

    LinearCodeConstraints() : GenerationConstraints(), candMinHD()
    {
    }

    LinearCodeConstraints(int min_hd) : GenerationConstraints(), candMinHD(min_hd)
    {
    }
};

struct VTCodeConstraints : public GenerationConstraints
{
    // --- VT Code Specific Parameters ---

    /**
     * @brief The remainder parameter 'a' for the VT code.
     * @details This integer value defines the specific VT code used for
     * candidate generation. Different values yield different sets of codewords.
     */
    int a;
    /**
     * @brief The remainder parameter 'b' for the VT code.
     */
    int b;

    VTCodeConstraints() : GenerationConstraints(), a(), b()
    {
    }

    VTCodeConstraints(int rem_a, int rem_b) : GenerationConstraints(), a(rem_a), b(rem_b)
    {
    }
};

struct AllStringsConstraints : public GenerationConstraints
{
    AllStringsConstraints() : GenerationConstraints()
    {
    }
};

struct Custom1Constraints : public GenerationConstraints
{
    // --- Custom Method 1 Specific Parameters ---
    //

    int remainder;

    Custom1Constraints() : GenerationConstraints(), remainder()
    {
    }
    Custom1Constraints(int rem) : GenerationConstraints(), remainder(rem)
    {
    }
};

struct Custom2Constraints : public GenerationConstraints
{
    // --- Custom Method 2 Specific Parameters ---
    //

    int remainder;

    Custom2Constraints() : GenerationConstraints(), remainder()
    {
    }
    Custom2Constraints(int rem) : GenerationConstraints(), remainder(rem)
    {
    }
};

/**
 * @struct ProgressiveWaveConstraints
 * @brief Constraints for the Progressive Wave Construction method.
 */
struct ProgressiveWaveConstraints : public GenerationConstraints
{
    /** @brief The number of diverse seed sequences to start the generation from. */
    int num_seeds;
    /** @brief The number of random candidates to generate for the initial pool. */
    int pool_size;

    ProgressiveWaveConstraints() : GenerationConstraints(), num_seeds(), pool_size()
    {
    }
    ProgressiveWaveConstraints(int seeds, int pool) : GenerationConstraints(), num_seeds(seeds), pool_size(pool)
    {
    }
};

/**
 * @struct RandomConstraints
 * @brief Constraints for the Random Generation method.
 */
struct RandomConstraints : public GenerationConstraints
{
    /** @brief The number of random candidates to generate. */
    int num_candidates;

    RandomConstraints() : GenerationConstraints(), num_candidates()
    {
    }
    RandomConstraints(int num) : GenerationConstraints(), num_candidates(num)
    {
    }
};

/**
 * @struct Params
 * @brief A structure to hold all configuration parameters for the codebook generation process.
 */
struct Params
{
    // --- Core Code Parameters ---

    /** @brief The length of the final codewords (e.g., 10, 16). */
    int codeLen;

    /**
     * @brief The minimum Levenshtein (edit) distance for the final codebook.
     * @details This is the primary goal of the algorithm: to produce a set of codewords
     * where any two words have at least this many insertions, deletions, or substitutions
     * between them.
     * @note Recommended values: {3, 4, 5}.
     */
    int codeMinED;

    // --- Generation Method & Constraints ---
    GenerationMethod method;
    std::unique_ptr<GenerationConstraints> constraints; // Holds the method-specific parameters

    // --- Biological Constraints ---

    /**
     * @brief The longest allowed homopolymer run (sequence of identical bases).
     * @details For example, a value of 3 would forbid "AAAA" but allow "AAA".
     * Set to 0 to disable this filter.
     */
    int maxRun;

    /**
     * @brief The minimum GC-content (fraction of G's and C's).
     * @details For example, a value of 0.3 means at least 30% of the bases must be G or C.
     * Set to 0 to disable the lower bound of this filter.
     */
    double minGCCont;

    /**
     * @brief The maximum GC-content (fraction of G's and C's).
     * @details For example, a value of 0.7 means at most 70% of the bases can be G or C.
     * Set to 0 to disable the upper bound of this filter.
     */
    double maxGCCont;

    // --- Performance & Execution Parameters ---

    /** @brief The number of threads to use for parallelizable tasks. */
    int threadNum;

    /**
     * @brief The interval in seconds at which to save progress to a file.
     * @details This allows for resuming a long-running generation process if it is interrupted.
     */
    int saveInterval;

    /** @brief Default constructor. Initializes all members to zero/default values. */
    Params()
        : codeLen(), codeMinED(), method(), constraints(), maxRun(), minGCCont(), maxGCCont(), threadNum(),
          saveInterval() {};

    /**
     * @brief Parameterized constructor for easy initialization.
     */
    Params(const int codeLen, const int minED, const int maxRun, const double minGCCont, const double maxGCCont,
           const int threadNum, const int saveInterval, GenerationMethod gen_method,
           std::unique_ptr<GenerationConstraints> &&constraints)
        : codeLen(codeLen), codeMinED(minED), method(gen_method), constraints(std::move(constraints)), maxRun(maxRun),
          minGCCont(minGCCont), maxGCCont(maxGCCont), threadNum(threadNum), saveInterval(saveInterval)
    {
    }
};

#endif /* INDEXGEN_HPP_ */
