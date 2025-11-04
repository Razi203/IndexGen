/**
 * @file CandidateGenerator.hpp
 * @brief Defines the abstract base class and derived classes for candidate generation.
 *
 * This file implements an OOP approach to candidate generation, where each generation
 * strategy is encapsulated in its own class derived from CandidateGenerator.
 */

#ifndef CANDIDATEGENERATOR_HPP_
#define CANDIDATEGENERATOR_HPP_

#include "IndexGen.hpp"
#include <fstream>
#include <memory>
#include <string>
#include <vector>

/**
 * @class CandidateGenerator
 * @brief Abstract base class for all candidate generation strategies.
 *
 * This class provides a common interface for generating candidates and printing
 * generation-specific information. Each derived class implements a specific
 * generation strategy (e.g., Linear Code, Random, VT Code, etc.).
 */
class CandidateGenerator
{
  protected:
    const Params &params; ///< Reference to the parameters structure

  public:
    /**
     * @brief Constructor that stores a reference to the parameters.
     * @param params The parameters structure containing all configuration.
     */
    explicit CandidateGenerator(const Params &params) : params(params)
    {
    }

    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~CandidateGenerator() = default;

    /**
     * @brief Pure virtual function to generate candidates.
     * @return A vector of strings representing the generated candidates (unfiltered).
     */
    virtual std::vector<std::string> generate() = 0;

    /**
     * @brief Pure virtual function to print generation method information.
     * @param output_stream The output stream to write information to (can be a file or cout).
     * @details This should output method name and any method-specific parameters.
     */
    virtual void printInfo(std::ostream &output_stream) const = 0;

    /**
     * @brief Pure virtual function to get the method name as a string.
     * @return The name of the generation method.
     */
    virtual std::string getMethodName() const = 0;

    /**
     * @brief Pure virtual function to print method-specific parameters to file.
     * @param output_file The output file stream to write parameters to.
     * @details Each parameter is printed as a number on a new line.
     */
    virtual void printParams(std::ofstream &output_file) const = 0;

    /**
     * @brief Pure virtual function to read method-specific parameters from file.
     * @param input_file The input file stream to read parameters from.
     * @param constraints Pointer to the constraints object to update with read values.
     * @details Reads parameters that were written by printParams and updates both
     *          the generator's internal state and the constraints object.
     */
    virtual void readParams(std::ifstream &input_file, GenerationConstraints *constraints) = 0;

    /**
     * @brief Apply filters to the generated candidates.
     * @param unfiltered The unfiltered candidates.
     * @return A vector of filtered candidates based on GC-content and max run constraints.
     */
    std::vector<std::string> applyFilters(const std::vector<std::string> &unfiltered) const;
};

/**
 * @class LinearCodeGenerator
 * @brief Generator using linear codes over GF(4) with guaranteed minimum Hamming distance.
 */
class LinearCodeGenerator : public CandidateGenerator
{
  private:
    int candMinHD; ///< Minimum Hamming distance for candidate generation

  public:
    /**
     * @brief Constructor for LinearCodeGenerator.
     * @param params The parameters structure.
     * @param constraints The linear code specific constraints.
     */
    LinearCodeGenerator(const Params &params, const LinearCodeConstraints &constraints);

    std::vector<std::string> generate() override;
    void printInfo(std::ostream &output_stream) const override;
    std::string getMethodName() const override;
    void printParams(std::ofstream &output_file) const override;
    void readParams(std::ifstream &input_file, GenerationConstraints *constraints) override;
};

/**
 * @class AllStringsGenerator
 * @brief Generator that produces all possible 4^n strings.
 */
class AllStringsGenerator : public CandidateGenerator
{
  public:
    /**
     * @brief Constructor for AllStringsGenerator.
     * @param params The parameters structure.
     */
    explicit AllStringsGenerator(const Params &params);

    std::vector<std::string> generate() override;
    void printInfo(std::ostream &output_stream) const override;
    std::string getMethodName() const override;
    void printParams(std::ofstream &output_file) const override;
    void readParams(std::ifstream &input_file, GenerationConstraints *constraints) override;
};

/**
 * @class RandomGenerator
 * @brief Generator that produces random candidates.
 */
class RandomGenerator : public CandidateGenerator
{
  private:
    int numCandidates; ///< Number of random candidates to generate

  public:
    /**
     * @brief Constructor for RandomGenerator.
     * @param params The parameters structure.
     * @param constraints The random generation specific constraints.
     */
    RandomGenerator(const Params &params, const RandomConstraints &constraints);

    std::vector<std::string> generate() override;
    void printInfo(std::ostream &output_stream) const override;
    std::string getMethodName() const override;
    void printParams(std::ofstream &output_file) const override;
    void readParams(std::ifstream &input_file, GenerationConstraints *constraints) override;
};

/**
 * @class VTCodeGenerator
 * @brief Generator using Varshamov-Tenengolts codes.
 */
class VTCodeGenerator : public CandidateGenerator
{
  private:
    int a; ///< VT code parameter 'a'
    int b; ///< VT code parameter 'b'

  public:
    /**
     * @brief Constructor for VTCodeGenerator.
     * @param params The parameters structure.
     * @param constraints The VT code specific constraints.
     */
    VTCodeGenerator(const Params &params, const VTCodeConstraints &constraints);

    std::vector<std::string> generate() override;
    void printInfo(std::ostream &output_stream) const override;
    std::string getMethodName() const override;
    void printParams(std::ofstream &output_file) const override;
    void readParams(std::ifstream &input_file, GenerationConstraints *constraints) override;
};

/**
 * @class DifferentialVTCodeGenerator
 * @brief Generator using Differential Varshamov-Tenengolts codes.
 */
class DifferentialVTCodeGenerator : public CandidateGenerator
{
  private:
    int syndrome; ///< Syndrome parameter for differential VT codes

  public:
    /**
     * @brief Constructor for DifferentialVTCodeGenerator.
     * @param params The parameters structure.
     * @param constraints The differential VT code specific constraints.
     */
    DifferentialVTCodeGenerator(const Params &params, const DifferentialVTCodeConstraints &constraints);

    std::vector<std::string> generate() override;
    void printInfo(std::ostream &output_stream) const override;
    std::string getMethodName() const override;
    void printParams(std::ofstream &output_file) const override;
    void readParams(std::ifstream &input_file, GenerationConstraints *constraints) override;
};

/**
 * @class RandomLinearGenerator
 * @brief Generator that randomly samples from linear code codewords.
 */
class RandomLinearGenerator : public CandidateGenerator
{
  private:
    int candMinHD;     ///< Minimum Hamming distance for the linear code
    int numCandidates; ///< Number of candidates to randomly select

  public:
    /**
     * @brief Constructor for RandomLinearGenerator.
     * @param params The parameters structure.
     * @param constraints The random linear specific constraints.
     */
    RandomLinearGenerator(const Params &params, const RandomLinearConstraints &constraints);

    std::vector<std::string> generate() override;
    void printInfo(std::ostream &output_stream) const override;
    std::string getMethodName() const override;
    void printParams(std::ofstream &output_file) const override;
    void readParams(std::ifstream &input_file, GenerationConstraints *constraints) override;
};

/**
 * @brief Factory function to create the appropriate generator based on params.
 * @param params The parameters structure containing method and constraints.
 * @return A unique pointer to the created generator.
 * @throws std::runtime_error if constraints are invalid or method is unknown.
 */
std::unique_ptr<CandidateGenerator> CreateGenerator(const Params &params);

#endif /* CANDIDATEGENERATOR_HPP_ */
