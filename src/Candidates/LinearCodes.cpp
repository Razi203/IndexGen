/**
 * @file LinearCodes.cpp
 * @brief Implementation for generating linear block codes over GF(4).
 */
#include "Candidates/LinearCodes.hpp" // Use the new documented header
#include "Candidates/GF4.hpp"         // Contains functions for GF(4) arithmetic (e.g., MatMulGF4)
#include "Candidates/GenMat.hpp"      // Contains pre-computed generator matrices
#include "Utils.hpp"                  // For utility functions like NextBase4
#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>

// --- Internal Helper Functions ---

/**
 * @brief Shuffle the columns of a given matrix according to a permutation.
 * @param mat The input matrix to be permuted.
 * @param perm The column permutation to apply.
 * @return The permuted matrix.
 */
vector<vector<int>> PermuteColumns(const vector<vector<int>> &mat, const vector<int> &perm)
{
    // If permutation is empty or identity, skip
    if (perm.empty())
        return mat;

    int rowNum = mat.size();
    int colNum = mat[0].size();

    // Print the permutation vector
    std::cout << "Permutation vector (columns): ";
    for (const int &val : perm)
        std::cout << val << " ";
    std::cout << std::endl;

    // Create the permuted matrix
    vector<vector<int>> permutedMat(rowNum, vector<int>(colNum));
    for (int i = 0; i < rowNum; i++)
    {
        for (int j = 0; j < colNum; j++)
        {
            permutedMat[i][j] = mat[i][perm[j]];
        }
    }
    return permutedMat;
}

/**
 * @brief Shuffle the rows of a given matrix according to a permutation.
 * @param mat The input matrix to be permuted.
 * @param perm The row permutation to apply.
 * @return The permuted matrix.
 */
vector<vector<int>> PermuteRows(const vector<vector<int>> &mat, const vector<int> &perm)
{
    // If permutation is empty or identity, skip
    if (perm.empty())
        return mat;

    int rowNum = mat.size();
    int colNum = mat[0].size();

    // Print the permutation vector
    std::cout << "Permutation vector (rows): ";
    for (const int &val : perm)
        std::cout << val << " ";
    std::cout << std::endl;

    // Create the permuted matrix
    vector<vector<int>> permutedMat(rowNum, vector<int>(colNum));
    for (int i = 0; i < rowNum; i++)
    {
        for (int j = 0; j < colNum; j++)
        {
            permutedMat[i][j] = mat[perm[i]][j];
        }
    }
    return permutedMat;
}

/**
 * @brief Adds a bias vector to a set of codewords.
 * @param codewords The input codewords to which the bias will be added.
 * @param bias The bias vector to be added to each codeword.
 * @return The codewords after adding the bias.
 */
vector<vector<int>> AddBias(const vector<vector<int>> &codewords, const vector<int> &bias)
{
    vector<vector<int>> result = codewords; // Copy the original codewords
    int n = bias.size();
    std::cout << "Adding bias: ";
    for (const int &b : bias)
        std::cout << b << " ";
    std::cout << std::endl;
    for (auto &codeword : result)
    {
        assert(codeword.size() == static_cast<long unsigned int>(n));
        for (int i = 0; i < n; i++)
        {
            codeword[i] = AddGF4(codeword[i], bias[i]);
        }
    }
    return result;
}

/**
 * @brief "Shortens" a linear code by removing the first `delNum` coordinates from its generator matrix.
 * @details Shortening is a standard technique in coding theory to derive a new code with slightly
 * different parameters from an existing one. Here, it effectively means we are only using a
 * sub-matrix of a larger, pre-defined generator matrix.
 * @param mat The original generator matrix.
 * @param delNum The number of initial rows and columns to delete.
 * @return The new, smaller generator matrix for the shortened code.
 */
vector<vector<int>> Shorten(const vector<vector<int>> &mat, const int delNum)
{
    // TODO: remove or change (results in bad performance and bad codes)
    // vector<vector<int>> mat = PermuteRows(PermuteColumns(mat_org));
    // vector<vector<int>> mat = PermuteColumns(mat_org);
    // vector<vector<int>> mat = PermuteRows(mat_org);
    assert(delNum >= 0);
    assert(not mat.empty());
    int rowNum = mat.size();
    int colNum = mat[0].size();
    assert(rowNum > delNum);
    assert(colNum > delNum);

    vector<vector<int>> result;
    for (int row = delNum; row < rowNum; row++)
    {
        vector<int> currRow(mat[row].begin() + delNum, mat[row].end());
        result.push_back(currRow);
    }
    return result;
}

/**
 * @brief Creates a generator matrix for a simple parity-check code with Hamming distance 2.
 * @param n The length of the code.
 * @return A generator matrix for an [n, n-1, 2] code.
 */
vector<vector<int>> GenMat2(const int n)
{
    vector<vector<int>> genMat(n - 1, vector<int>(n, 0)); // Initialize with zeros
    // (n-1)x(n-1) identity matrix
    for (int i = 0; i < n - 1; i++)
        genMat[i][i] = 1;
    // Final column of 1's for the parity check
    for (int i = 0; i < n - 1; i++)
        genMat[i][n - 1] = 1;
    return genMat;
}

/**
 * @brief Creates a generator matrix for a code with Hamming distance 3 by shortening a known (21,18,3) code.
 * @param n The desired length of the code (4 <= n <= 21).
 * @return The generator matrix.
 */
vector<vector<int>> GenMat3(const int n)
{
    assert((n >= 4) and (n <= 21));
    int d = 21 - n;
    vector<vector<int>> genMat = Shorten(Gen_21_18_3(), d);
    return genMat;
}

/**
 * @brief Creates a generator matrix for a code with Hamming distance 4 by shortening a known (41,36,4) code.
 * @param n The desired length of the code (6 <= n <= 41).
 * @return The generator matrix.
 */
vector<vector<int>> GenMat4(const int n)
{
    assert((n >= 6) and (n <= 41));
    int d = 41 - n;
    vector<vector<int>> genMat = Shorten(Gen_41_36_4(), d);
    return genMat;
}

/**
 * @brief Creates a generator matrix for a code with Hamming distance 5 by shortening a known (43,36,5) code.
 * @param n The desired length of the code (8 <= n <= 43).
 * @return The generator matrix.
 */
vector<vector<int>> GenMat5(const int n)
{
    assert((n >= 8) and (n <= 43));
    int d = 43 - n;
    vector<vector<int>> genMat = Shorten(Gen_43_36_5(), d);
    return genMat;
}

/**
 * @brief Encodes a set of raw data vectors into codewords with a specific Hamming distance.
 * @param rawVecs The input data vectors.
 * @param codedVecs The output vector to store the resulting codewords.
 * @param n The length of the output codewords.
 * @param minHammDist The target minimum Hamming distance.
 * @param bias The bias vector to add to each codeword.
 * @param row_perm The row permutation for the generator matrix.
 * @param col_perm The column permutation for the generator matrix.
 */
void CodeVecs(const vector<vector<int>> &rawVecs, vector<vector<int>> &codedVecs, const int n, const int minHammDist,
              const vector<int> &bias, const vector<int> &row_perm, const vector<int> &col_perm)
{
    assert((minHammDist >= 2) && (minHammDist <= 5));
    vector<vector<int>> genMat;
    int k = 0; // The dimension of the code (length of raw vectors)

    switch (minHammDist)
    {
    case 2:
        genMat = GenMat2(n);
        k = n - 1;
        break;
    case 3:
        genMat = GenMat3(n);
        k = n - 3;
        break;
    case 4:
        genMat = GenMat4(n);
        k = n - 5;
        break;
    case 5:
        genMat = GenMat5(n);
        k = n - 7;
        break;
    default:
        assert(0); // Should be unreachable
    }

    // Apply permutations using provided vectors
    genMat = PermuteColumns(genMat, col_perm);
    genMat = PermuteRows(genMat, row_perm);

    // Determine if bias should be applied
    bool useBias = !bias.empty();
    if (useBias)
    {
        std::cout << "Using bias: ";
        for (const int &b : bias)
            std::cout << b << " ";
        std::cout << std::endl;
    }
    else
    {
        std::cout << "Not using bias addition." << std::endl;
    }
    std::cout << std::endl;

    // Multiply each raw data vector by the generator matrix to get the codeword
    for (const vector<int> &rawVec : rawVecs)
    {
        assert((int)rawVec.size() == k);
        vector<int> codedVec = MatMulGF4(rawVec, genMat, k, n);

        // Apply bias if provided
        if (useBias)
        {
            for (int i = 0; i < n; i++)
            {
                codedVec[i] = AddGF4(codedVec[i], bias[i]);
            }
        }

        codedVecs.push_back(codedVec);
    }
}

/**
 * @brief Generates all possible data vectors of a given length over GF(4).
 * @details This function enumerates every possible vector of length `dataLen`
 * with elements from {0, 1, 2, 3}. This is equivalent to counting from 0 to 4^dataLen - 1
 * in base 4.
 * @param dataLen The length of the vectors to generate.
 * @return A vector containing all 4^dataLen possible data vectors.
 */
vector<vector<int>> DataVecs(const int dataLen)
{
    vector<vector<int>> result;
    if (dataLen <= 0)
        return result;

    vector<int> start(dataLen, 0);
    for (vector<int> vec = start; not vec.empty(); vec = NextBase4(vec))
    {
        result.push_back(vec);
    }
    return result;
}

// --- Public Function Implementation ---

// See LinearCodes.hpp for function documentation.
vector<vector<int>> CodedVecs(const int n, const int minHammDist, const vector<int> &bias, const vector<int> &row_perm,
                              const vector<int> &col_perm)
{
    assert((minHammDist >= 2) && (minHammDist <= 5));
    // Determine the dimension 'k' of the data vectors based on the desired
    // code length 'n' and minimum Hamming distance. This relationship is
    // specific to the pre-computed generator matrices being used.
    int k = 0;
    switch (minHammDist)
    {
    case 2:
        k = n - 1;
        break;
    case 3:
        k = n - 3;
        break;
    case 4:
        k = n - 5;
        break;
    case 5:
        k = n - 7;
        break;
    }

    // 1. Generate all possible raw data vectors of length k.
    vector<vector<int>> rawVecs = DataVecs(k);

    // 2. Encode these raw vectors into codewords of length n.
    vector<vector<int>> codedVecs;
    codedVecs.reserve(rawVecs.size()); // Pre-allocate memory for efficiency
    CodeVecs(rawVecs, codedVecs, n, minHammDist, bias, row_perm, col_perm);
    return codedVecs;
}

// Backward compatibility overload with default parameters
vector<vector<int>> CodedVecs(const int n, const int minHammDist)
{
    // Use default parameters: zero bias and identity permutations
    vector<int> bias(n, 0);
    vector<int> row_perm, col_perm;

    // Calculate row permutation size
    int k = 0;
    switch (minHammDist)
    {
    case 2:
        k = n - 1;
        break;
    case 3:
        k = n - 3;
        break;
    case 4:
        k = n - 5;
        break;
    case 5:
        k = n - 7;
        break;
    }

    // Generate identity permutations
    row_perm.resize(k);
    col_perm.resize(n);
    for (int i = 0; i < k; ++i)
        row_perm[i] = i;
    for (int i = 0; i < n; ++i)
        col_perm[i] = i;

    return CodedVecs(n, minHammDist, bias, row_perm, col_perm);
}
