/**
 * @file MaxClique.cpp
 * @brief Implementation of the maximum clique algorithm for codebook generation.
 */

#include "MaxClique.hpp"
#include <vector>
#include "Utils.hpp"
#include "Candidates.hpp"
#include "mcqd.h" // External library for solving the max clique problem

/**
 * @brief Allocates a 2D boolean matrix of size n x n.
 * @param n The dimension of the square matrix.
 * @return A pointer to the allocated n x n matrix, initialized to all false.
 */
bool **AllocateMatrix(const int n)
{
	// Allocate rows
	bool **result = new bool *[n];
	for (int i = 0; i < n; ++i)
		result[i] = new bool[n];

	// Initialize all entries to false
	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++)
			result[i][j] = false;
	return result;
}

/**
 * @brief Deallocates a 2D matrix that was created with AllocateMatrix.
 * @param n The dimension of the square matrix.
 * @param mat The matrix to delete.
 */
void DeleteMatrix(const int n, bool **mat)
{
	for (int i = 0; i < n; ++i)
		delete[] mat[i];
	delete[] mat;
}

/**
 * @brief Constructs an adjacency matrix for the compatibility graph.
 * @details An edge (i, j) is set to `true` if the edit distance between string i and
 * string j is greater than or equal to `minED`.
 * @param strs The vector of candidate strings.
 * @param minED The minimum edit distance required for two strings to be compatible.
 * @param matrixOnesNum A reference to a long long to store the number of non-edges (conflicts).
 * @return A pointer to the newly created adjacency matrix.
 */
bool **AdjMatrix(const std::vector<std::string> &strs, const int minED, long long int &matrixOnesNum)
{
	int n = strs.size();
	bool **m = AllocateMatrix(n);
	long long int conflictCount = 0;

	// Iterate over the upper triangle of the matrix
	for (int i = 0; i < n; i++)
	{
		for (int j = i + 1; j < n; j++)
		{
			bool isCompatible = FastEditDistance(strs[i], strs[j], minED);
			if (isCompatible)
			{
				// If compatible, add an edge in both directions
				m[i][j] = true;
				m[j][i] = true;
			}
			else
			{
				// If not compatible, they are in conflict.
				conflictCount += 2;
			}
		}
	}
	matrixOnesNum = conflictCount; // Note: This variable name is misleading here; it stores conflicts.
	return m;
}

/**
 * @brief Finds the maximum clique and translates it into a codebook of strings.
 * @param params The generation parameters.
 * @param candidates The vector of candidate strings.
 * @param codebook An output vector to store the resulting codebook strings.
 * @param matrixOnesNum A reference to store the number of conflicts.
 */
void MaxCliqueStrings(const Params &params, const std::vector<std::string> &candidates, std::vector<std::string> &codebook,
					  long long int &matrixOnesNum)
{
	bool **mat = AdjMatrix(candidates, params.codeMinED, matrixOnesNum);
	int size = candidates.size();

	// Use the external library to find the maximum clique
	Maxclique m(mat, size);
	int qsize; // Will hold the size of the clique
	int *qmax; // Will hold the indices of the clique members

	// m.mcq(qmax, qsize); // A basic algorithm
	m.mcqdyn(qmax, qsize); // A dynamic, usually faster, algorithm

	// Convert the indices from the clique back to strings
	codebook.reserve(qsize);
	for (int i = 0; i < qsize; i++)
		codebook.push_back(candidates[qmax[i]]);

	DeleteMatrix(size, mat);
}

// See MaxClique.hpp for function documentation.
void GenerateCodebookMaxClique(const Params &params)
{
	PrintTestParams(params);
	std::vector<std::string> candidates = Candidates(params);
	std::vector<std::string> codebook;
	long long int matrixOnesNum;
	int candidateNum = candidates.size();

	MaxCliqueStrings(params, candidates, codebook, matrixOnesNum);

	PrintTestResults(candidateNum, matrixOnesNum, codebook.size());

	ToFile(codebook, params, candidateNum, matrixOnesNum);
	VerifyDist(codebook, params.codeMinED, params.threadNum);
	std::cout << "=====================================================" << std::endl;
}
