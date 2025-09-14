#include <vector>
#include "Utils.hpp"
#include "Candidates.hpp"
#include "mcqd.h"

bool** AllocateMatrix(const int n) {
	// Allocate
	bool** result = new bool*[n];
	for (int i = 0; i < n; ++i)
		result[i] = new bool[n];
	// Initialize
	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++)
			result[i][j] = false;
	return result;
}

void DeleteMatrix(const int n, bool** mat) {
	for (int i = 0; i < n; ++i)
		delete[] mat[i];
	delete[] mat;
}

bool** AdjMatrix(const vector<string>& strs, const int minED, long long int& matrixOnesNum) {
	int n = strs.size();
	bool** m = AllocateMatrix(n);
	long long int countOnes = 0;
	for (int i = 0; i < n; i++) {
		for (int j = i + 1; j < n; j++) {
			bool EDAtLeastMinED = FastEditDistance(strs[i], strs[j], minED);
			if (EDAtLeastMinED) {
				m[i][j] = true;
				m[j][i] = true;
			}
			else {
				countOnes += 2;
			}
		}
	}
	matrixOnesNum = countOnes;
	return m;
}

void MaxCliqueStrings(const Params& params, const vector<string>& candidates, vector<string>& codebook,
		long long int& matrixOnesNum) {
	bool** mat = AdjMatrix(candidates, params.codeMinED, matrixOnesNum);
	int size = candidates.size();
	Maxclique m(mat, size);
	int qsize;
	int* qmax;
//	m.mcq(qmax, qsize);
	m.mcqdyn(qmax, qsize); // usually faster
	for (int i = 0; i < qsize; i++)
		codebook.push_back(candidates[qmax[i]]);
	DeleteMatrix(size, mat);
}

void GenerateCodebookMaxClique(const Params& params) {
	PrintTestParams(params);
	vector<string> candidates = Candidates(params);
	vector<string> codebook;
	long long int matrixOnesNum;
	int candidateNum = candidates.size();
	MaxCliqueStrings(params, candidates, codebook, matrixOnesNum);

	PrintTestResults(candidateNum, matrixOnesNum, codebook.size());

	ToFile(codebook, params, candidateNum, matrixOnesNum);
	VerifyDist(codebook, params.codeMinED, params.threadNum);
	cout << "=====================================================" << endl;
}

//	bool** conn = AdjMat1();
//	int size = 5;
//	int qsize;
//	int* qmax;
//	Maxclique m(conn, size);
//	m.mcq(qmax, qsize);
//
//	cout << "==================" << endl;
//	for (int i = 0; i < qsize; i++)
//		cout << qmax[i] << endl;
