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

/**
 * @enum GenerationMethod
 * @brief Identifies the algorithm used for codebook generation.
 */
enum class GenerationMethod
{
	LINEAR_CODE, ///< Uses linear codes to generate candidate strings with guaranteed Hamming distance.
	VT_CODE,	 ///< Uses Varshamov-Tenengolts codes for candidate generation.
	ALL_STRINGS	 ///< Generates all possible strings of the specified length (brute-force).
};

/**
 * @struct GenerationConstraints
 * @brief Base class for method-specific generation constraints.
 */
struct GenerationConstraints
{
	virtual ~GenerationConstraints() = default; // Ensures derived class destructors are called

	GenerationConstraints() {}
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

	LinearCodeConstraints() : GenerationConstraints(), candMinHD() {}

	LinearCodeConstraints(int min_hd) : GenerationConstraints(), candMinHD(min_hd) {}
};

struct VTCodeConstraints : public GenerationConstraints
{
	// --- VT Code Specific Parameters ---

	/**
	 * @brief The remainder parameter for the VT code.
	 * @details This integer value (0 to n) defines the specific VT code used for
	 * candidate generation. Different values yield different sets of codewords.
	 * @note Valid values: {0, 1, ..., n}, where n is
	 * the length of the codewords.
	 */
	int remainder;

	VTCodeConstraints() : GenerationConstraints(), remainder() {}

	VTCodeConstraints(int rem) : GenerationConstraints(), remainder(rem) {}
};

struct AllStringsConstraints : public GenerationConstraints
{
	AllStringsConstraints() : GenerationConstraints() {}
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
	Params() : codeLen(), codeMinED(), method(), constraints(), maxRun(), minGCCont(), maxGCCont(), threadNum(), saveInterval() {};

	/**
	 * @brief Parameterized constructor for easy initialization.
	 */
	Params(const int codeLen, const int minED, const int maxRun,
		   const double minGCCont, const double maxGCCont, const int threadNum, const int saveInterval, GenerationMethod gen_method, std::unique_ptr<GenerationConstraints> &&constraints) : codeLen(codeLen), codeMinED(minED), method(gen_method), constraints(std::move(constraints)), maxRun(maxRun), minGCCont(minGCCont), maxGCCont(maxGCCont), threadNum(threadNum), saveInterval(saveInterval)
	{
	}
};

#endif /* INDEXGEN_HPP_ */
