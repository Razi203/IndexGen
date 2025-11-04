/**
 * @file CandidateGenerator.cpp
 * @brief Implements the CandidateGenerator base class and all derived classes.
 */

#include "CandidateGenerator.hpp"
#include "Candidates/DifferentialVTCodes.hpp"
#include "Candidates/LinearCodes.hpp"
#include "Candidates/RandomLinear.hpp"
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
    : CandidateGenerator(params), candMinHD(constraints.candMinHD)
{
}

std::vector<std::string> LinearCodeGenerator::generate()
{
    vector<vector<int>> codeVecs = CodedVecs(params.codeLen, candMinHD);
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
}

std::string LinearCodeGenerator::getMethodName() const
{
    return "LinearCode";
}

void LinearCodeGenerator::printParams(std::ofstream &output_file) const
{
    output_file << candMinHD << '\n';
}

void LinearCodeGenerator::readParams(std::ifstream &input_file, GenerationConstraints *constraints)
{
    input_file >> candMinHD;
    if (auto *lc = dynamic_cast<LinearCodeConstraints *>(constraints))
    {
        lc->candMinHD = candMinHD;
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
// RandomLinearGenerator Implementation
// ============================================================================

RandomLinearGenerator::RandomLinearGenerator(const Params &params, const RandomLinearConstraints &constraints)
    : CandidateGenerator(params), candMinHD(constraints.candMinHD), numCandidates(constraints.num_candidates)
{
}

std::vector<std::string> RandomLinearGenerator::generate()
{
    return GenerateRandomLinearCandidates(params.codeLen, candMinHD, numCandidates);
}

void RandomLinearGenerator::printInfo(std::ostream &output_stream) const
{
    output_stream << "Using Generation Method: RandomLinear (minHD=" << candMinHD << ", candidates=" << numCandidates << ")"
                  << endl;
}

std::string RandomLinearGenerator::getMethodName() const
{
    return "RandomLinear";
}

void RandomLinearGenerator::printParams(std::ofstream &output_file) const
{
    output_file << candMinHD << '\n';
    output_file << numCandidates << '\n';
}

void RandomLinearGenerator::readParams(std::ifstream &input_file, GenerationConstraints *constraints)
{
    input_file >> candMinHD;
    input_file >> numCandidates;
    if (auto *rlc = dynamic_cast<RandomLinearConstraints *>(constraints))
    {
        rlc->candMinHD = candMinHD;
        rlc->num_candidates = numCandidates;
    }
}

// ============================================================================
// Factory Function Implementation
// ============================================================================

std::unique_ptr<CandidateGenerator> CreateGenerator(const Params &params)
{
    if (!params.constraints)
    {
        throw std::runtime_error("Cannot create generator: constraints object is null.");
    }

    switch (params.method)
    {
    case GenerationMethod::LINEAR_CODE:
    {
        auto *constraints = dynamic_cast<LinearCodeConstraints *>(params.constraints.get());
        if (!constraints)
        {
            throw std::runtime_error("Invalid constraints provided for LINEAR_CODE method.");
        }
        return std::make_unique<LinearCodeGenerator>(params, *constraints);
    }

    case GenerationMethod::ALL_STRINGS:
    {
        return std::make_unique<AllStringsGenerator>(params);
    }

    case GenerationMethod::RANDOM:
    {
        auto *constraints = dynamic_cast<RandomConstraints *>(params.constraints.get());
        if (!constraints)
        {
            throw std::runtime_error("Invalid constraints provided for RANDOM method.");
        }
        return std::make_unique<RandomGenerator>(params, *constraints);
    }

    case GenerationMethod::VT_CODE:
    {
        auto *constraints = dynamic_cast<VTCodeConstraints *>(params.constraints.get());
        if (!constraints)
        {
            throw std::runtime_error("Invalid constraints provided for VT_CODE method.");
        }
        return std::make_unique<VTCodeGenerator>(params, *constraints);
    }

    case GenerationMethod::DIFFERENTIAL_VT_CODE:
    {
        auto *constraints = dynamic_cast<DifferentialVTCodeConstraints *>(params.constraints.get());
        if (!constraints)
        {
            throw std::runtime_error("Invalid constraints provided for DIFFERENTIAL_VT_CODE method.");
        }
        return std::make_unique<DifferentialVTCodeGenerator>(params, *constraints);
    }

    case GenerationMethod::RANDOM_LINEAR:
    {
        auto *constraints = dynamic_cast<RandomLinearConstraints *>(params.constraints.get());
        if (!constraints)
        {
            throw std::runtime_error("Invalid constraints provided for RANDOM_LINEAR method.");
        }
        return std::make_unique<RandomLinearGenerator>(params, *constraints);
    }

    default:
    {
        throw std::runtime_error("Unknown or unsupported candidate generation method.");
    }
    }
}
