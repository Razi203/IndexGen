/**
 * @file CandidateGenerator.cpp
 * @brief Implements the CandidateGenerator base class and all derived classes.
 */

#include "CandidateGenerator.hpp"
#include "Candidates/DifferentialVTCodes.hpp"
#include "Candidates/LinearCodes.hpp"
#include "Candidates/VTCodes.hpp"
#include "Utils.hpp"
#include <fstream>
#include <iostream>
#include <random>
#include <thread>

using namespace std;

// ============================================================================
// Base Class Implementation
// ============================================================================

std::vector<std::string> CandidateGenerator::applyFilters(const std::vector<std::string> &unfiltered) const
{
    std::vector<std::string> filtered;

    bool useMaxRunFilter = (params.maxRun > 0);
    bool useGCFilter = (params.minGCCont > 0 || params.maxGCCont > 0);

    // No filters case
    if (!useMaxRunFilter && !useGCFilter)
    {
        return unfiltered;
    }

    // Apply filters
    for (const std::string &str : unfiltered)
    {
        bool passMaxRun = !useMaxRunFilter || (MaxRun(str) <= params.maxRun);
        bool passGC = !useGCFilter || TestGCCont(str, params.minGCCont, params.maxGCCont);

        if (passMaxRun && passGC)
        {
            filtered.push_back(str);
        }
    }

    return filtered;
}

// ============================================================================
// LinearCodeGenerator Implementation
// ============================================================================

LinearCodeGenerator::LinearCodeGenerator(const Params &params, const LinearCodeConstraints &constraints)
    : CandidateGenerator(params), candMinHD(constraints.candMinHD), bias(constraints.bias), row_perm(constraints.row_perm),
      col_perm(constraints.col_perm), bias_mode(constraints.bias_mode), row_perm_mode(constraints.row_perm_mode),
      col_perm_mode(constraints.col_perm_mode), random_seed(constraints.random_seed), vectors_initialized(false)
{
}

void LinearCodeGenerator::initializeVectors(int code_len)
{
    // Calculate expected sizes for this code length
    int col_perm_size = code_len;
    int row_perm_size = calculateRowPermSize(code_len, candMinHD);
    int bias_size = code_len;

    // Check if vectors are already initialized and have the correct size
    bool correct_size = vectors_initialized &&
                        (bias.empty() || (int)bias.size() == bias_size) &&
                        (row_perm.empty() || (int)row_perm.size() == row_perm_size) &&
                        (col_perm.empty() || (int)col_perm.size() == col_perm_size);

    if (correct_size)
        return; // Already initialized with correct sizes

    // Setup random number generator
    mt19937 rng;
    if (random_seed == 0)
    {
        // Generate a seed from current time and store it for reproducibility
        random_seed = static_cast<unsigned int>(chrono::system_clock::now().time_since_epoch().count());
    }
    rng.seed(random_seed);

    // Initialize bias
    if (bias_mode == VectorMode::DEFAULT)
    {
        bias.assign(bias_size, 0); // Zero vector
    }
    else if (bias_mode == VectorMode::RANDOM)
    {
        bias = generateRandomBias(bias_size, rng);
    }
    else
    { // MANUAL
        // For manual mode, if size doesn't match, this is an error
        if ((int)bias.size() != bias_size)
        {
            throw std::runtime_error("Manual bias vector has size " + std::to_string(bias.size()) +
                                   " but code length " + std::to_string(code_len) +
                                   " requires size " + std::to_string(bias_size) +
                                   ". Please provide vectors for each code length.");
        }
        validateBias(bias, bias_size);
    }

    // Initialize row permutation
    if (row_perm_mode == VectorMode::DEFAULT)
    {
        row_perm = generateIdentityPerm(row_perm_size);
    }
    else if (row_perm_mode == VectorMode::RANDOM)
    {
        row_perm = generateRandomPerm(row_perm_size, rng);
    }
    else
    { // MANUAL
        if ((int)row_perm.size() != row_perm_size)
        {
            throw std::runtime_error("Manual row_perm has size " + std::to_string(row_perm.size()) +
                                   " but code length " + std::to_string(code_len) +
                                   " with minHD " + std::to_string(candMinHD) +
                                   " requires size " + std::to_string(row_perm_size));
        }
        validatePermutation(row_perm, row_perm_size, "row_perm");
    }

    // Initialize column permutation
    if (col_perm_mode == VectorMode::DEFAULT)
    {
        col_perm = generateIdentityPerm(col_perm_size);
    }
    else if (col_perm_mode == VectorMode::RANDOM)
    {
        col_perm = generateRandomPerm(col_perm_size, rng);
    }
    else
    { // MANUAL
        if ((int)col_perm.size() != col_perm_size)
        {
            throw std::runtime_error("Manual col_perm has size " + std::to_string(col_perm.size()) +
                                   " but code length " + std::to_string(code_len) +
                                   " requires size " + std::to_string(col_perm_size));
        }
        validatePermutation(col_perm, col_perm_size, "col_perm");
    }

    vectors_initialized = true;

    // Note: The actual seed used (if it was 0) is now stored in random_seed
    // and will be saved via printParams() for reproducibility
}

std::vector<std::string> LinearCodeGenerator::generate()
{
    // Initialize vectors now that we know code_len
    initializeVectors(params.codeLen);

    // Generate code with vectors
    vector<vector<int>> codeVecs = CodedVecs(params.codeLen, candMinHD, bias, row_perm, col_perm);
    vector<string> codeWords;
    for (const vector<int> &vec : codeVecs)
    {
        codeWords.push_back(VecToStr(vec));
    }
    return codeWords;
}

void LinearCodeGenerator::printInfo(std::ostream &output_stream) const
{
    output_stream << "Using Generation Method: LinearCode (minHD=" << candMinHD << ")" << endl;

    // Print bias vector
    if (!bias.empty())
    {
        output_stream << "Bias vector: [";
        for (size_t i = 0; i < bias.size(); ++i)
        {
            output_stream << bias[i];
            if (i < bias.size() - 1)
                output_stream << ", ";
        }
        output_stream << "]" << endl;
    }
    else
    {
        output_stream << "Bias vector: (not initialized)" << endl;
    }

    // Print row permutation vector
    if (!row_perm.empty())
    {
        output_stream << "Row permutation: [";
        for (size_t i = 0; i < row_perm.size(); ++i)
        {
            output_stream << row_perm[i];
            if (i < row_perm.size() - 1)
                output_stream << ", ";
        }
        output_stream << "]" << endl;
    }
    else
    {
        output_stream << "Row permutation: (not initialized)" << endl;
    }

    // Print column permutation vector
    if (!col_perm.empty())
    {
        output_stream << "Column permutation: [";
        for (size_t i = 0; i < col_perm.size(); ++i)
        {
            output_stream << col_perm[i];
            if (i < col_perm.size() - 1)
                output_stream << ", ";
        }
        output_stream << "]" << endl;
    }
    else
    {
        output_stream << "Column permutation: (not initialized)" << endl;
    }
}

std::string LinearCodeGenerator::getMethodName() const
{
    return "LinearCode";
}

void LinearCodeGenerator::printParams(std::ofstream &output_file) const
{
    output_file << candMinHD << '\n';

    // Write vector modes (as integers)
    output_file << static_cast<int>(bias_mode) << '\n';
    output_file << static_cast<int>(row_perm_mode) << '\n';
    output_file << static_cast<int>(col_perm_mode) << '\n';

    // Write random seed
    output_file << random_seed << '\n';

    // Write bias vector: size,val1,val2,...
    output_file << bias.size();
    for (int val : bias)
        output_file << "," << val;
    output_file << '\n';

    // Write row permutation: size,val1,val2,...
    output_file << row_perm.size();
    for (int val : row_perm)
        output_file << "," << val;
    output_file << '\n';

    // Write column permutation: size,val1,val2,...
    output_file << col_perm.size();
    for (int val : col_perm)
        output_file << "," << val;
    output_file << '\n';
}

void LinearCodeGenerator::readParams(std::ifstream &input_file, GenerationConstraints *constraints)
{
    input_file >> candMinHD;
    input_file.ignore(); // Skip newline

    // Read vector modes
    int bias_mode_int, row_perm_mode_int, col_perm_mode_int;
    input_file >> bias_mode_int;
    input_file >> row_perm_mode_int;
    input_file >> col_perm_mode_int;
    bias_mode = static_cast<VectorMode>(bias_mode_int);
    row_perm_mode = static_cast<VectorMode>(row_perm_mode_int);
    col_perm_mode = static_cast<VectorMode>(col_perm_mode_int);

    // Read random seed
    input_file >> random_seed;
    input_file.ignore(); // Skip newline

    // Read vectors
    string line;

    // Read bias
    getline(input_file, line);
    if (!line.empty())
    {
        size_t comma_pos = line.find(',');
        if (comma_pos != string::npos)
        {
            bias = parseCSVVector(line.substr(comma_pos + 1), "bias");
        }
        else
        {
            bias.clear();
        }
    }

    // Read row_perm
    getline(input_file, line);
    if (!line.empty())
    {
        size_t comma_pos = line.find(',');
        if (comma_pos != string::npos)
        {
            row_perm = parseCSVVector(line.substr(comma_pos + 1), "row_perm");
        }
        else
        {
            row_perm.clear();
        }
    }

    // Read col_perm
    getline(input_file, line);
    if (!line.empty())
    {
        size_t comma_pos = line.find(',');
        if (comma_pos != string::npos)
        {
            col_perm = parseCSVVector(line.substr(comma_pos + 1), "col_perm");
        }
        else
        {
            col_perm.clear();
        }
    }

    // Mark as initialized so initializeVectors won't regenerate
    // But if code length changes, we'll need to reinitialize
    vectors_initialized = false; // Set to false to allow re-initialization for new code length

    // Update constraints if provided
    if (auto *lc = dynamic_cast<LinearCodeConstraints *>(constraints))
    {
        lc->candMinHD = candMinHD;
        lc->bias = bias;
        lc->row_perm = row_perm;
        lc->col_perm = col_perm;
        lc->bias_mode = bias_mode;
        lc->row_perm_mode = row_perm_mode;
        lc->col_perm_mode = col_perm_mode;
        lc->random_seed = random_seed;
    }
}

// ============================================================================
// AllStringsGenerator Implementation
// ============================================================================

AllStringsGenerator::AllStringsGenerator(const Params &params) : CandidateGenerator(params)
{
}

std::vector<std::string> AllStringsGenerator::generate()
{
    vector<string> result;
    vector<int> start(params.codeLen, 0);
    // Iterate through all base-4 numbers of length n
    for (vector<int> vec = start; not vec.empty(); vec = NextBase4(vec))
    {
        result.push_back(VecToStr(vec));
    }
    return result;
}

void AllStringsGenerator::printInfo(std::ostream &output_stream) const
{
    output_stream << "Using Generation Method: AllStrings" << endl;
}

std::string AllStringsGenerator::getMethodName() const
{
    return "AllStrings";
}

void AllStringsGenerator::printParams(std::ofstream &output_file) const
{
    // AllStrings has no specific parameters to save
}

void AllStringsGenerator::readParams(std::ifstream &input_file, GenerationConstraints *constraints)
{
    // AllStrings has no specific parameters to read
    (void)input_file;    // Suppress unused parameter warning
    (void)constraints;   // Suppress unused parameter warning
}

// ============================================================================
// RandomGenerator Implementation
// ============================================================================

RandomGenerator::RandomGenerator(const Params &params, const RandomConstraints &constraints)
    : CandidateGenerator(params), numCandidates(constraints.num_candidates)
{
}

std::vector<std::string> RandomGenerator::generate()
{
    vector<string> result;
    result.reserve(numCandidates);

    int n = params.codeLen;
    int threadNum = params.threadNum;

    // Calculate candidates per thread
    int candidates_per_thread = numCandidates / threadNum;
    int remainder = numCandidates % threadNum;

    // Vector to store threads and their results
    vector<std::thread> threads;
    vector<vector<string>> thread_results(threadNum);

    // Lambda function for each thread to generate random strings
    auto generate_random = [n](int count, int seed, vector<string> &local_result)
    {
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> dist(0, 3);
        local_result.reserve(count);

        for (int i = 0; i < count; ++i)
        {
            string random_str;
            random_str.reserve(n);
            for (int j = 0; j < n; ++j)
            {
                random_str += std::to_string(dist(rng));
            }
            local_result.push_back(random_str);
        }
    };

    // Launch threads
    for (int i = 0; i < threadNum; ++i)
    {
        int count = candidates_per_thread + (i < remainder ? 1 : 0);
        int seed = std::random_device{}() + i;
        threads.emplace_back(generate_random, count, seed, std::ref(thread_results[i]));
    }

    // Wait for all threads to complete
    for (auto &thread : threads)
    {
        thread.join();
    }

    // Combine results from all threads
    for (const auto &local_result : thread_results)
    {
        result.insert(result.end(), local_result.begin(), local_result.end());
    }

    return result;
}

void RandomGenerator::printInfo(std::ostream &output_stream) const
{
    output_stream << "Using Generation Method: Random (candidates=" << numCandidates << ")" << endl;
}

std::string RandomGenerator::getMethodName() const
{
    return "Random";
}

void RandomGenerator::printParams(std::ofstream &output_file) const
{
    output_file << numCandidates << '\n';
}

void RandomGenerator::readParams(std::ifstream &input_file, GenerationConstraints *constraints)
{
    input_file >> numCandidates;
    if (auto *rc = dynamic_cast<RandomConstraints *>(constraints))
    {
        rc->num_candidates = numCandidates;
    }
}

// ============================================================================
// VTCodeGenerator Implementation
// ============================================================================

VTCodeGenerator::VTCodeGenerator(const Params &params, const VTCodeConstraints &constraints)
    : CandidateGenerator(params), a(constraints.a), b(constraints.b)
{
}

std::vector<std::string> VTCodeGenerator::generate()
{
    return GenerateVTCodes(params.codeLen, a, b, params.threadNum);
}

void VTCodeGenerator::printInfo(std::ostream &output_stream) const
{
    output_stream << "Using Generation Method: VTCode (a=" << a << ", b=" << b << ")" << endl;
}

std::string VTCodeGenerator::getMethodName() const
{
    return "VTCode";
}

void VTCodeGenerator::printParams(std::ofstream &output_file) const
{
    output_file << a << '\n';
    output_file << b << '\n';
}

void VTCodeGenerator::readParams(std::ifstream &input_file, GenerationConstraints *constraints)
{
    input_file >> a;
    input_file >> b;
    if (auto *vtc = dynamic_cast<VTCodeConstraints *>(constraints))
    {
        vtc->a = a;
        vtc->b = b;
    }
}

// ============================================================================
// DifferentialVTCodeGenerator Implementation
// ============================================================================

DifferentialVTCodeGenerator::DifferentialVTCodeGenerator(const Params &params,
                                                         const DifferentialVTCodeConstraints &constraints)
    : CandidateGenerator(params), syndrome(constraints.syndrome)
{
}

std::vector<std::string> DifferentialVTCodeGenerator::generate()
{
    return GenerateDifferentialVTCodes(params.codeLen, syndrome, params.threadNum);
}

void DifferentialVTCodeGenerator::printInfo(std::ostream &output_stream) const
{
    output_stream << "Using Generation Method: Differential VTCode (syndrome=" << syndrome << ")" << endl;
}

std::string DifferentialVTCodeGenerator::getMethodName() const
{
    return "DifferentialVTCode";
}

void DifferentialVTCodeGenerator::printParams(std::ofstream &output_file) const
{
    output_file << syndrome << '\n';
}

void DifferentialVTCodeGenerator::readParams(std::ifstream &input_file, GenerationConstraints *constraints)
{
    input_file >> syndrome;
    if (auto *dvtc = dynamic_cast<DifferentialVTCodeConstraints *>(constraints))
    {
        dvtc->syndrome = syndrome;
    }
}

// ============================================================================
// Factory Function Implementation (Singleton Pattern)
// ============================================================================

std::shared_ptr<CandidateGenerator> CreateGenerator(const Params &params)
{
    // Static cache to store the singleton instance
    static std::shared_ptr<CandidateGenerator> cached_generator = nullptr;
    static const Params* cached_params = nullptr;

    // If we already have a cached generator for the same params object, return it
    // We compare pointer addresses since Params objects should be stable during a run
    if (cached_generator && cached_params == &params)
    {
        return cached_generator;
    }

    if (!params.constraints)
    {
        throw std::runtime_error("Cannot create generator: constraints object is null.");
    }

    // Create new generator
    std::shared_ptr<CandidateGenerator> new_generator;

    switch (params.method)
    {
    case GenerationMethod::LINEAR_CODE:
    {
        auto *constraints = dynamic_cast<LinearCodeConstraints *>(params.constraints.get());
        if (!constraints)
        {
            throw std::runtime_error("Invalid constraints provided for LINEAR_CODE method.");
        }
        new_generator = std::make_shared<LinearCodeGenerator>(params, *constraints);
        break;
    }

    case GenerationMethod::ALL_STRINGS:
    {
        new_generator = std::make_shared<AllStringsGenerator>(params);
        break;
    }

    case GenerationMethod::RANDOM:
    {
        auto *constraints = dynamic_cast<RandomConstraints *>(params.constraints.get());
        if (!constraints)
        {
            throw std::runtime_error("Invalid constraints provided for RANDOM method.");
        }
        new_generator = std::make_shared<RandomGenerator>(params, *constraints);
        break;
    }

    case GenerationMethod::VT_CODE:
    {
        auto *constraints = dynamic_cast<VTCodeConstraints *>(params.constraints.get());
        if (!constraints)
        {
            throw std::runtime_error("Invalid constraints provided for VT_CODE method.");
        }
        new_generator = std::make_shared<VTCodeGenerator>(params, *constraints);
        break;
    }

    case GenerationMethod::DIFFERENTIAL_VT_CODE:
    {
        auto *constraints = dynamic_cast<DifferentialVTCodeConstraints *>(params.constraints.get());
        if (!constraints)
        {
            throw std::runtime_error("Invalid constraints provided for DIFFERENTIAL_VT_CODE method.");
        }
        new_generator = std::make_shared<DifferentialVTCodeGenerator>(params, *constraints);
        break;
    }

    default:
    {
        throw std::runtime_error("Unknown or unsupported candidate generation method.");
    }
    }

    // Cache the new generator and params pointer
    cached_generator = new_generator;
    cached_params = &params;

    return cached_generator;
}
