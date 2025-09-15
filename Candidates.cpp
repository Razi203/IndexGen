/**
 * @file Candidates.cpp
 * @brief Implements the logic for generating and filtering candidate codewords.
 */

#include "Candidates.hpp"
#include "Utils.hpp"
#include "LinearCodes.hpp"
#include <cassert>
#include <iostream>

// --- Internal Generation and Filtering Functions ---

/**
 * @brief Generates all possible strings of length n over the alphabet {0, 1, 2, 3}.
 * @param n The desired length of the strings.
 * @return A vector containing all 4^n possible strings.
 */
vector<string> GenAllStrings(const int n)
{
	vector<string> result;
	vector<int> start(n, 0);
	// Iterate through all base-4 numbers of length n
	for (vector<int> vec = start; not vec.empty(); vec = NextBase4(vec))
	{
		result.push_back(VecToStr(vec));
	}
	return result;
}

/**
 * @brief Generates all codewords of length n with a specified minimum Hamming distance.
 * @details This function uses the `LinearCodes` module to create a set of strings that
 * are guaranteed to be separated by at least `minHammDist`.
 * @param n The length of the codewords.
 * @param minHammDist The minimum Hamming distance (from 2 to 5).
 * @return A vector of strings representing the generated codewords.
 */
vector<string> GenAllCodeStrings(const int n, const int minHammDist)
{
	vector<vector<int>> codeVecs = CodedVecs(n, minHammDist);
	vector<string> codeWords;
	for (const vector<int> &vec : codeVecs)
	{
		codeWords.push_back(VecToStr(vec));
	}
	return codeWords;
}

/**
 * @brief A pass-through filter that does nothing.
 * @param strs The input strings.
 * @param filteredStrs The output vector, which will be a copy of the input.
 * @param params Unused.
 */
void NoFilter(const vector<string> &strs, vector<string> &filteredStrs, const Params &params)
{
	filteredStrs = strs;
}

/**
 * @brief Filters strings based on GC-content.
 * @details Keeps only the strings where the fraction of '1's (C) and '2's (G) is
 * within the specified range.
 * @param strs The input strings.
 * @param filteredStrs The output vector containing only strings that pass the filter.
 * @param params The parameter struct containing minGCCont and maxGCCont.
 */
void FilterGC(const vector<string> &strs, vector<string> &filteredStrs, const Params &params)
{
	for (const string &str : strs)
	{
		if (TestGCCont(str, params.minGCCont, params.maxGCCont))
		{
			filteredStrs.push_back(str);
		}
	}
}

/**
 * @brief Filters strings based on maximum homopolymer run length.
 * @details Keeps only the strings where the longest sequence of identical consecutive
 * characters is less than or equal to `params.maxRun`.
 * @param strs The input strings.
 * @param filteredStrs The output vector containing only strings that pass the filter.
 * @param params The parameter struct containing `maxRun`.
 */
void FilterMaxRun(const vector<string> &strs, vector<string> &filteredStrs, const Params &params)
{
	for (const string &str : strs)
	{
		if ((MaxRun(str) <= params.maxRun))
		{
			filteredStrs.push_back(str);
		}
	}
}

/**
 * @brief Filters strings based on both GC-content and maximum run length.
 * @param strs The input strings.
 * @param filteredStrs The output vector containing only strings that pass both filters.
 * @param params The parameter struct containing all filter criteria.
 */
void FilterGCMaxRun(const vector<string> &strs, vector<string> &filteredStrs, const Params &params)
{
	for (const string &str : strs)
	{
		if ((MaxRun(str) <= params.maxRun) && TestGCCont(str, params.minGCCont, params.maxGCCont))
		{
			filteredStrs.push_back(str);
		}
	}
}

// --- Public Function Implementations ---

// See Candidates.hpp for function documentation.
vector<string> Candidates(const Params &params)
{
	int minHD = params.candMinHD;
	vector<string> unfiltered, filtered;

	// Step 1: Generate the initial set of strings.
	if (minHD <= 1)
	{ // No minimum Hamming distance required, generate all possible strings.
		unfiltered = GenAllStrings(params.codeLen);
	}
	else if (minHD >= 2 && minHD <= 5)
	{ // Use a linear code for a Hamming distance guarantee.
		unfiltered = GenAllCodeStrings(params.codeLen, params.candMinHD);
	}
	else
	{
		assert(0); // Invalid minHD parameter.
	}

	// Step 2: Apply the specified filters.
	bool useMaxRunFilter = (params.maxRun > 0);
	bool useGCFilter = (params.minGCCont > 0 || params.maxGCCont > 0);

	if (useMaxRunFilter && useGCFilter)
	{
		FilterGCMaxRun(unfiltered, filtered, params);
	}
	else if (useMaxRunFilter)
	{
		FilterMaxRun(unfiltered, filtered, params);
	}
	else if (useGCFilter)
	{
		FilterGC(unfiltered, filtered, params);
	}
	else
	{
		NoFilter(unfiltered, filtered, params); // No filters applied.
	}
	return filtered;
}

// See Candidates.hpp for function documentation.
void TestCandidates(const int n, const int d)
{
	vector<string> cand = GenAllCodeStrings(n, d);
	cout << "Testing code n=" << n << "\td=" << d << "\tcode size " << cand.size() << "...";

	// Verify that the Hamming distance is correct for all pairs.
	bool success = true;
	for (size_t i = 0; i < cand.size(); i++)
	{
		for (size_t j = i + 1; j < cand.size(); j++)
		{
			if (HammingDist(cand[i], cand[j]) < d)
			{
				success = false;
				break;
			}
		}
		if (!success)
			break;
	}

	if (!success)
	{
		cout << "FAILURE" << endl;
	}
	else
	{
		cout << "SUCCESS" << endl;
	}
}
