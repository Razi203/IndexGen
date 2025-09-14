#ifndef DECODE_HPP_
#define DECODE_HPP_

#include <vector>
#include <string>

using namespace std;

void Decode(const vector<string>& codebook, const vector<string>& receivedWords, vector<string>& decodedWords,
		const int codeLen, const int threadNum);

void TestDecode(const int testNum, const int wordNum, const string& codebookFilename, const int threadNum);

#endif /* DECODE_HPP_ */
