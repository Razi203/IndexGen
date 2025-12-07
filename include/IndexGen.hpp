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
    LINEAR_CODE,          ///< Uses linear codes to generate candidate strings with guaranteed Hamming distance.
    ALL_STRINGS,          ///< Generates all possible strings of the specified length (brute-force).
    RANDOM,               ///< Generates candidates using a randomization approach.
    VT_CODE,              ///< Uses Varshamov-Tenengolts codes for candidate generation.
    DIFFERENTIAL_VT_CODE  ///< Uses Differential Varshamov-Tenengolts codes for candidate generation.
};

/**
 * @enum VectorMode
 * @brief Specifies how vectors (bias, row_perm, col_perm) should be initialized.
 */
enum class VectorMode
{
    DEFAULT,   ///< Zero for bias, identity for permutations
    RANDOM,    ///< Randomly generated
    MANUAL     ///< User-specified
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

    // --- Vector Values ---
    std::vector<int> bias;        ///< Bias vector (length = code length)
    std::vector<int> row_perm;    ///< Row permutation vector
    std::vector<int> col_perm;    ///< Column permutation vector

    // --- Vector Modes ---
    VectorMode bias_mode;         ///< How to initialize bias
    VectorMode row_perm_mode;     ///< How to initialize row permutation
    VectorMode col_perm_mode;     ///< How to initialize column permutation

    // --- Random Seed ---
    unsigned int random_seed;     ///< Random seed (0 = use time-based)

    LinearCodeConstraints()
        : GenerationConstraints(), candMinHD(), bias(), row_perm(), col_perm(),
          bias_mode(VectorMode::DEFAULT), row_perm_mode(VectorMode::DEFAULT), col_perm_mode(VectorMode::DEFAULT),
          random_seed(0)
    {
    }

    LinearCodeConstraints(int min_hd)
        : GenerationConstraints(), candMinHD(min_hd), bias(), row_perm(), col_perm(),
          bias_mode(VectorMode::DEFAULT), row_perm_mode(VectorMode::DEFAULT), col_perm_mode(VectorMode::DEFAULT),
          random_seed(0)
    {
    }

    LinearCodeConstraints(int min_hd, VectorMode bias_m, VectorMode row_perm_m, VectorMode col_perm_m,
                         std::vector<int> biasVec, std::vector<int> rowPerm, std::vector<int> colPerm,
                         unsigned int seed)
        : GenerationConstraints(), candMinHD(min_hd), bias(biasVec), row_perm(rowPerm), col_perm(colPerm),
          bias_mode(bias_m), row_perm_mode(row_perm_m), col_perm_mode(col_perm_m), random_seed(seed)
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

struct DifferentialVTCodeConstraints : public GenerationConstraints
{
    int syndrome; // The syndrome parameter

    DifferentialVTCodeConstraints() : syndrome()
    {
    }

    DifferentialVTCodeConstraints(int syndrome) : syndrome(syndrome)
    {
        // Assuming BaseConstraints has a virtual destructor or function
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

    /** @brief Whether to verify the codebook distance after generation. */
    bool verify;

    /** @brief Whether to use GPU acceleration for adjacency list generation. */
    bool useGPU;

    /** @brief Maximum GPU memory to use in GB. */
    double maxGPUMemoryGB;

    /** @brief Default constructor. Initializes all members to zero/default values. */
    Params()
        : codeLen(), codeMinED(), method(), constraints(), maxRun(), minGCCont(), maxGCCont(), threadNum(),
          saveInterval(), verify(false), useGPU(true), maxGPUMemoryGB(10.0) {};

    /**
     * @brief Parameterized constructor for easy initialization.
     */
    Params(const int codeLen, const int minED, const int maxRun, const double minGCCont, const double maxGCCont,
           const int threadNum, const int saveInterval, GenerationMethod gen_method,
           std::unique_ptr<GenerationConstraints> &&constraints, bool verify = false, bool useGPU = true, double maxGPUMemoryGB = 10.0)
        : codeLen(codeLen), codeMinED(minED), method(gen_method), constraints(std::move(constraints)), maxRun(maxRun),
          minGCCont(minGCCont), maxGCCont(maxGCCont), threadNum(threadNum), saveInterval(saveInterval), verify(verify),
          useGPU(useGPU), maxGPUMemoryGB(maxGPUMemoryGB)
    {
    }
};

#endif /* INDEXGEN_HPP_ */
