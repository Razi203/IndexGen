#include <cassert>
#include <iostream>
#include "LinearCodes.hpp"
#include "GenMat.hpp"
#include "GF4.hpp"
#include "Utils.hpp"

// delete first delNum rows and first delNum columns from matrix

vector<vector<int> > Shorten(const vector<vector<int> >& mat, const int delNum) {
	assert(delNum >= 0);
	assert(not mat.empty());
	int rowNum = mat.size();
	int colNum = mat[0].size();
	assert(rowNum > delNum);
	assert(colNum > delNum);

	vector<vector<int> > result;
	for (int row = delNum; row < rowNum; row++) {
		vector<int> currRow(mat[row].begin() + delNum, mat[row].end());
		result.push_back(currRow);
	}
	return result;
}

// Generator matrix for the parity code [n,n-1,2]: (n-1)x(n-1) identity matrix + column of -1 (Roth page 27)
vector<vector<int> > GenMat2(const int n) {
	vector<vector<int> > genMat(n - 1, vector<int>(n));
	// (n-1)x(n-1) identity matrix
	for (int i = 0; i < n - 1; i++)
		genMat[i][i] = 1;
	// column of -1's
	for (int i = 0; i < n - 1; i++)
		genMat[i][n - 1] = 1;
	return genMat;
}

// Generator matrix for (n,n-3,3) by shortening code (21,18,3)
vector<vector<int> > GenMat3(const int n) {
	assert((n >= 4) and (n <= 21));
	int d = 21 - n;
	vector<vector<int> > genMat = Shorten(Gen_21_18_3(), d);
	return genMat;
}

// Generator matrix for (n,n-5,4) by shortening code (41,36,4)
vector<vector<int> > GenMat4(const int n) {
	assert((n >= 6) and (n <= 41));
	int d = 41 - n;
	vector<vector<int> > genMat = Shorten(Gen_41_36_4(), d);
	return genMat;
}

// Generator matrix for (n,n-7,5) by shortening code (43,36,5)
vector<vector<int> > GenMat5(const int n) {
	assert((n >= 8) and (n <= 43));
	int d = 43 - n;
	vector<vector<int> > genMat = Shorten(Gen_43_36_5(), d);
	return genMat;
}

// n: length of codedVecs.
// rawVecs: vector of int vectors from {0,1,2,3} of size n-1
// codedVecs: vector of int vectors from {0,1,2,3} of size n, s.t. Hamming distance between any two is at least 2
void Code2(const vector<vector<int> >& rawVecs, vector<vector<int> >& codedVecs, const int n) {
	assert(n > 1);
	vector<vector<int> > genMat = GenMat2(n);
	for (const vector<int>& rawVec : rawVecs) {
		assert((int ) rawVec.size() == n - 1);
		vector<int> codedVec = MatMulGF4(rawVec, genMat, n - 1, n);
		codedVecs.push_back(codedVec);
	}
}

// n: length of codedVecs. 4<=n<=21
// rawVecs: vector of int vectors from {0,1,2,3} of size n-3
// codedVecs: vector of int vectors from {0,1,2,3} of size n, s.t. Hamming distance between any two is at least 3
void Code3(const vector<vector<int> >& rawVecs, vector<vector<int> >& codedVecs, const int n) {
	assert((n >= 4) and (n <= 21));
	vector<vector<int> > genMat = GenMat3(n);
	for (const vector<int>& rawVec : rawVecs) {
		assert((int ) rawVec.size() == n - 3);
		vector<int> codedVec = MatMulGF4(rawVec, genMat, n - 3, n);
		codedVecs.push_back(codedVec);
	}
}

// n: length of codedVecs. 6<=n<=41
// rawVecs: vector of int vectors from {0,1,2,3} of size n-5
// codedVecs: vector of int vectors from {0,1,2,3} of size n, s.t. Hamming distance between any two is at least 4
void Code4(const vector<vector<int> >& rawVecs, vector<vector<int> >& codedVecs, const int n) {
	assert((n >= 6) and (n <= 41));
	vector<vector<int> > genMat = GenMat4(n);
	for (const vector<int>& rawVec : rawVecs) {
		assert((int ) rawVec.size() == n - 5);
		vector<int> codedVec = MatMulGF4(rawVec, genMat, n - 5, n);
		codedVecs.push_back(codedVec);
	}
}

// n: length of codedVecs. 8<=n<=43
// rawVecs: vector of int vectors from {0,1,2,3} of size n-7
// codedVecs: vector of int vectors from {0,1,2,3} of size n, s.t. Hamming distance between any two is at least 5
void Code5(const vector<vector<int> >& rawVecs, vector<vector<int> >& codedVecs, const int n) {
	assert((n >= 8) and (n <= 43));
	vector<vector<int> > genMat = GenMat5(n);
	for (const vector<int>& rawVec : rawVecs) {
		assert((int ) rawVec.size() == n - 7);
		vector<int> codedVec = MatMulGF4(rawVec, genMat, n - 7, n);
		codedVecs.push_back(codedVec);
	}
}

// n: length of codedVecs:	minHammDist 3: 4<=n<=21
//							minHammDist 4: 6<=n<=41
//							minHammDist 5: 8<=n<=43
// rawVecs: vector of int vectors from {0,1,2,3} of size:	minHammDist 3: n-3
//															minHammDist 4: n-5
//															minHammDist 5: n-7
// codedVecs: vector of int vectors from {0,1,2,3} of size n, s.t. Hamming distance between any two is at least minHammDist from {3,4,5}
void CodeVecs(const vector<vector<int> >& rawVecs, vector<vector<int> >& codedVecs, const int n,
		const int minHammDist) {
	assert((minHammDist >= 2) && (minHammDist <= 5));
	switch (minHammDist) {
	case 2:
		Code2(rawVecs, codedVecs, n);
		break;
	case 3:
		Code3(rawVecs, codedVecs, n);
		break;
	case 4:
		Code4(rawVecs, codedVecs, n);
		break;
	case 5:
		Code5(rawVecs, codedVecs, n);
		break;
	default:
		assert(0);
	}
}

// Generate Min Hamming Distance Vectors

vector<vector<int> > DataVecs(const int dataLen) {
	vector<vector<int> > result;
	vector<int> start(dataLen, 0);
	for (vector<int> vec = start; not vec.empty(); vec = NextBase4(vec)) {
		result.push_back(vec);
	}
	return result;
}

// codedVecs: All vectors of int vectors from {0,1,2,3} of size n, s.t. Hamming distance between any two is at least minHammDist from {2,3,4,5}
vector<vector<int> > CodedVecs(const int n, const int minHammDist) {
	assert((minHammDist >= 2) && (minHammDist <= 5));
	int k = n - (2 * minHammDist - 3);
	vector<vector<int> > rawVecs = DataVecs(k);
	vector<vector<int> > codedVecs;
	CodeVecs(rawVecs, codedVecs, n, minHammDist);
	return codedVecs;
}

