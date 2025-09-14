#include <cassert>
#include <iostream>
#include "Candidates.hpp"
#include "Utils.hpp"
#include "LinearCodes.hpp"

// All strings over {0(A),1(C),2(G),3(T)} of length n (NO MINIMUM HAMMING DISTANCE)
vector<string> GenAllStrings(const int n) {
	vector<string> result;
	vector<int> start(n, 0);
	for (vector<int> vec = start; not vec.empty(); vec = NextBase4(vec)) {
		result.push_back(VecToStr(vec));
	}
	return result;
}

// All strings over {0(A),1(C),2(G),3(T)} of length n with pairwise minimum Hamming distance minHammDist {3,4,5}
//n: length of codedVecs:	minHammDist 2: n>1
//							minHammDist 3: 4<=n<=21
//							minHammDist 4: 6<=n<=41
//							minHammDist 5: 8<=n<=43
vector<string> GenAllCodeStrings(const int n, const int minHammDist) {
	vector<vector<int> > codeVecs = CodedVecs(n, minHammDist);
	vector<string> codeWords;
	for (vector<int>& vec : codeVecs) {
		codeWords.push_back(VecToStr(vec));
	}
	return codeWords;
}

void NoFilter(const vector<string>& strs, vector<string>& filteredStrs, const Params& params) {
	for (const string& str : strs) {
		filteredStrs.push_back(str);
	}
}

void FilterGC(const vector<string>& strs, vector<string>& filteredStrs, const Params& params) {
	for (const string& str : strs) {
		if (TestGCCont(str, params.minGCCont, params.maxGCCont)) {
			filteredStrs.push_back(str);
		}
	}
}

void FilterMaxRun(const vector<string>& strs, vector<string>& filteredStrs, const Params& params) {
	for (const string& str : strs) {
		if ((MaxRun(str) <= params.maxRun)) {
			filteredStrs.push_back(str);
		}
	}
}

void FilterGCMaxRun(const vector<string>& strs, vector<string>& filteredStrs, const Params& params) {
	for (const string& str : strs) {
		if ((MaxRun(str) <= params.maxRun) && TestGCCont(str, params.minGCCont, params.maxGCCont)) {
			filteredStrs.push_back(str);
		}
	}
}

vector<string> Candidates(const Params& params) {
	int minHD = params.candMinHD;
	vector<string> unfiltered, filtered;
	// Create candidates. Code words or All strings.
	if (minHD == 1) { // No Hamming. All Strings
		unfiltered = GenAllStrings(params.codeLen);
	}
	else if (minHD == 2 or minHD == 3 or minHD == 4 or minHD == 5) { // Hamming Code
		unfiltered = GenAllCodeStrings(params.codeLen, params.candMinHD);
	}
	else {
		assert(0);
	}

	// Filter candidates
	bool TestMaxRun = (params.maxRun != 0);
	bool TestGC = (params.minGCCont != 0);
	if (TestMaxRun and TestGC) {
		FilterGCMaxRun(unfiltered, filtered, params);
	}
	else if (TestMaxRun) {
		FilterMaxRun(unfiltered, filtered, params);
	}
	else if (TestGC) {
		FilterGC(unfiltered, filtered, params);
	}
	else {
		NoFilter(unfiltered, filtered, params);
	}
	return filtered;
}

// test if min Hamming distance of code is at least d
bool TestHammDist(const vector<string>& code, const int d) {
	for (unsigned i = 0; i < code.size(); i++) {
		for (unsigned j = i + 1; j < code.size(); j++) {
			if (HammingDist(code[i], code[j]) < d) {
				return false;
			}
		}
	}
	return true;
}

void TestCandidates(const int n, const int d) {
	vector<string> cand = GenAllCodeStrings(n, d);
	cout << "Testing code n=" << n << "\td=" << d << "\tcode size " << cand.size() << "...";
	if (not TestHammDist(cand, d)) {
		cout << "FAILURE" << endl;
	}
	else {
		cout << "SUCCESS" << endl;
	}
}
