/**
 * @file Decode.cpp
 * @brief Implements the nearest-neighbor decoding algorithm.
 */

#include "Decode.hpp" // Use the new documented header
#include "Utils.hpp"
#include <fstream>
#include <iostream>
#include <cassert>
#include <unordered_set>
#include <thread>
#include <iomanip>
#include <chrono> // For seeding random number generator

// --- Internal Helper Functions ---

/**
 * @brief Loads a codebook from a text file.
 * @details Skips over a metadata header block (lines not starting with '=') and then reads
 * each subsequent line as a codeword.
 * @param filename The path to the codebook file.
 * @param codebook The output vector to store the loaded codewords.
 */
void CodebookFromFile(const string &filename, vector<string> &codebook)
{
	ifstream input;
	input.open(filename.c_str());
	if (!input.is_open())
	{
		cout << "Failed opening input file!" << endl;
		return;
	}
	string line;
	// Pass through the metadata header
	while (getline(input, line))
	{
		assert(not line.empty());
		if (line[0] == '=')
			break;
	}
	// Read the codewords
	while (getline(input, line))
	{
		codebook.push_back(line);
	}
	cout << "Read " << codebook.size() << " words from file" << endl;
	input.close();
}

/**
 * @brief Finds the single closest codeword in the codebook to a given received word.
 * @details This is the core nearest-neighbor search. It iterates through the entire codebook,
 * calculating the edit distance for each entry and keeping track of the minimum distance found.
 * It uses an optimized edit distance function (`FastEditDistance0123ForSearch`) that can
 * terminate early if the distance is guaranteed to be greater than the current minimum.
 * @param codebook The vector of all valid codewords.
 * @param cont0123Code Pre-computed character counts for each codeword to speed up distance calculation.
 * @param receivedWord The word to find the closest match for.
 * @param cont0123Rec Pre-computed character counts for the received word.
 * @param codeLen The length of the codewords.
 * @return The codeword from the codebook that has the minimum edit distance to `receivedWord`.
 */
string ClosestWord(const vector<string> &codebook, const vector<vector<char>> &cont0123Code,
				   const string &receivedWord, const vector<char> &cont0123Rec, const int codeLen)
{
	int codeSize = codebook.size();
	int minED = codeLen + 1;
	int closestCodewordIdx = -1;
	for (int i = 0; i < codeSize; i++)
	{
		// Use the optimized search that exits early if ED will be >= minED
		int currED = FastEditDistance0123ForSearch(receivedWord, codebook[i], minED, cont0123Rec, cont0123Code[i]);
		if (currED != -1)
		{ // A new minimum distance was found
			minED = currED;
			closestCodewordIdx = i;
			if (minED <= 1)
			{ // Optimization: if distance is 1 or 0, we can't do better.
				break;
			}
		}
	}
	assert(closestCodewordIdx != -1); // Should always find at least one match
	return codebook[closestCodewordIdx];
}

/**
 * @brief A wrapper function that first checks for an exact match before performing a full search.
 * @param codebook The vector of all valid codewords.
 * @param cont0123Code Pre-computed character counts for the codebook.
 * @param codebookSet An unordered_set of the codebook for fast O(1) average time lookups.
 * @param receivedWord The word to decode.
 * @param cont0123Rec Pre-computed character counts for the received word.
 * @param codeLen The length of the codewords.
 * @return The decoded codeword. Returns the `receivedWord` itself if it's an exact match, otherwise
 * calls `ClosestWord` to perform a full search.
 */
string NearestCodeword(const vector<string> &codebook, const vector<vector<char>> &cont0123Code,
					   const unordered_set<string> &codebookSet, const string &receivedWord, const vector<char> &cont0123Rec,
					   const int codeLen)
{
	// Optimization: If the received word is already a valid codeword, return it immediately.
	if (codebookSet.count(receivedWord))
	{
		return receivedWord;
	}
	return ClosestWord(codebook, cont0123Code, receivedWord, cont0123Rec, codeLen);
}

/**
 * @brief The worker function for each thread in the parallelized decoding.
 * @details Each thread is assigned a subset of the `receivedWords` to process. It iterates
 * through its assigned words (e.g., thread 0 takes words 0, N, 2N, ...; thread 1 takes 1, N+1, 2N+1, ...)
 * and calls `NearestCodeword` for each one.
 * @param codebook The main codebook (read-only).
 * @param receivedWords The main list of received words (read-only).
 * @param decodedWords The main output list (write target).
 * @param codebookSet The hash set of the codebook (read-only).
 * @param cont0123Code Pre-computed character counts (read-only).
 * @param cont0123Received Pre-computed character counts (read-only).
 * @param codeLen Length of codewords.
 * @param threadNum Total number of threads.
 * @param threadId The ID of the current thread (from 0 to threadNum-1).
 */
void DecodeThread(const vector<string> &codebook, const vector<string> &receivedWords, vector<string> &decodedWords,
				  const unordered_set<string> &codebookSet, const vector<vector<char>> &cont0123Code,
				  const vector<vector<char>> &cont0123Received, const int codeLen, const int threadNum, const int threadId)
{
	int receivedWordsNum = receivedWords.size();
	for (int i = threadId; i < receivedWordsNum; i += threadNum)
	{
		decodedWords[i] = NearestCodeword(codebook, cont0123Code, codebookSet, receivedWords[i], cont0123Received[i],
										  codeLen);
	}
}

// --- Public Function Implementations ---

// See Decode.hpp for function documentation.
void Decode(const vector<string> &codebook, const vector<string> &receivedWords, vector<string> &decodedWords,
			const int codeLen, const int threadNum)
{

	int receivedWordsNum = receivedWords.size();
	decodedWords.resize(receivedWordsNum);

	// Pre-computation steps for optimization
	vector<vector<char>> cont0123Code = Cont0123(codebook);
	vector<vector<char>> cont0123Received = Cont0123(receivedWords);
	unordered_set<string> codebookSet(codebook.begin(), codebook.end());

	if (threadNum <= 1)
	{ // Use a single thread
		for (int i = 0; i < receivedWordsNum; i++)
		{
			decodedWords[i] = NearestCodeword(codebook, cont0123Code, codebookSet, receivedWords[i], cont0123Received[i], codeLen);
		}
	}
	else
	{ // Use multiple threads
		vector<thread> threads;
		for (int threadId = 0; threadId < threadNum; threadId++)
		{
			threads.push_back(
				thread(DecodeThread, ref(codebook), ref(receivedWords), ref(decodedWords), ref(codebookSet), ref(cont0123Code),
					   ref(cont0123Received), codeLen, threadNum, threadId));
		}
		for (thread &th : threads)
		{
			th.join();
		}
	}
}

// --- Test-Related Helper Functions ---

/**
 * @brief Generates a vector of random words composed of characters {'0', '1', '2', '3'}.
 * @param wordNum The number of words to generate.
 * @param wordLen The length of each word.
 * @param words The output vector to store the random words.
 * @param generator The Mersenne Twister random number generator.
 */
void RandomWords(const int wordNum, const int wordLen, vector<string> &words, mt19937 &generator)
{
	words.resize(wordNum);
	for (int i = 0; i < wordNum; i++)
	{
		words[i] = MakeStrand0123(wordLen, generator);
	}
}

/**
 * @brief Finds the closest codeword using a simple, un-optimized brute-force search.
 * @details This function is used as a baseline to verify the correctness of the optimized `ClosestWord`.
 * @param codebook The vector of codewords.
 * @param receivedWord The word to decode.
 * @return The closest codeword.
 */
string ClosestCodewordBrute(const vector<string> &codebook, const string &receivedWord)
{
	int minED = receivedWord.size() + 1;
	int minCodewordIdx = -1;
	for (size_t i = 0; i < codebook.size(); i++)
	{
		int currED = FastEditDistance(codebook[i], receivedWord);
		if (currED < minED)
		{
			minED = currED;
			minCodewordIdx = i;
		}
	}
	assert(minCodewordIdx != -1);
	return codebook[minCodewordIdx];
}

/**
 * @brief Compares the results of the optimized decoder against the brute-force decoder.
 * @param codebook The codebook.
 * @param receivedWords The original list of received words.
 * @param decodedWords The words produced by the main `Decode` function.
 * @return True if the edit distance for every decoded word is the same as the brute-force result, False otherwise.
 */
bool TestDecodedWords(const vector<string> &codebook, const vector<string> &receivedWords,
					  const vector<string> &decodedWords)
{
	for (size_t i = 0; i < receivedWords.size(); i++)
	{
		int edClosest = FastEditDistance(receivedWords[i], decodedWords[i]);
		string closestCodewordBrute = ClosestCodewordBrute(codebook, receivedWords[i]);
		int edClosestBrute = FastEditDistance(receivedWords[i], closestCodewordBrute);
		if (edClosestBrute < edClosest)
		{
			cerr << "Decoding error found for word: " << receivedWords[i] << endl;
			cerr << "Optimized decoder found: " << decodedWords[i] << " (distance " << edClosest << ")" << endl;
			cerr << "Brute force found: " << closestCodewordBrute << " (distance " << edClosestBrute << ")" << endl;
			return false;
		}
	}
	return true;
}

// See Decode.hpp for function documentation.
void TestDecode(const int testNum, const int wordNum, const string &codebookFilename, const int threadNum)
{
	unsigned sd = chrono::high_resolution_clock::now().time_since_epoch().count();
	mt19937 generator(sd);
	vector<string> codebook, receivedWords, decodedWords;
	CodebookFromFile(codebookFilename, codebook);
	assert(not codebook.empty());
	int codeLen = codebook[0].size();

	for (int i = 0; i < testNum; i++)
	{
		RandomWords(wordNum, codeLen, receivedWords, generator);
		Decode(codebook, receivedWords, decodedWords, codeLen, threadNum);
		bool result = TestDecodedWords(codebook, receivedWords, decodedWords);
		if (not result)
		{
			cout << "Test Decode Failure!" << endl;
			return;
		}
	}
	cout << "Test Decode Success!" << endl;
}
