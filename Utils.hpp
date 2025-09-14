#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <string>
#include <vector>
#include <atomic>
#include <random>
#include "IndexGen.hpp"
using namespace std;

bool FastEditDistance0123(const string& X, const string& Y, const int minED, const vector<char>& contx,
		const vector<char>& conty);
bool HammingDist(const string& str1, const string& str2, const int minHammDist, const int threadIdx,
		const int threadNum);
int FastEditDistance(const string& source, const string& target);
bool FastEditDistance(const string& source, const string& target, const int minED);
int FastEditDistance0123ForSearch(const string& X, const string& Y, const int minED, const vector<char>& contx,
		const vector<char>& conty);
int FastEditDistanceForSearch(const string& source, const string& target, const int minED);

int HammingDist(const string& str1, const string& str2);
void VerifyDist(vector<string>& vecs, const int minED, const int threadNum);
void VerifyHammDist(const vector<string>& vecs, const int minHammDist);

int MaxRun(const string& str);
bool TestGCCont(const string& a, const double minGCCont, const double maxGCCont);
bool TestAllLettersOccurence(const string& a);

vector<int> NextBase4(const vector<int>& vec);
vector<int> MatMul(const vector<int>& v, const vector<vector<int> >& M, const int k, const int l);
int MulF4(const int a, const int b);
int AddF4(const int a, const int b);

string VecToStr(const vector<int>& vec);

void PrintTestParams(const Params& params);
void PrintTestResults(const int candidateNum, const long long int matrixOnesNum, const int codewordsNum);

void ToFile(const vector<string>& codeWords, const Params& params, const int candidateNum,
		const long long int matrixOnesNum);
void ParamsToFile(const Params& params, const string& fileName);
void FileToParams(Params& params, const string& fileName);
void IntVecToFile(const vector<int>& data, const string& fileName);
void FileToIntVec(vector<int>& data, const string& fileName);
void StrVecToFile(const vector<string>& data, const string& fileName);
void FileToStrVec(vector<string>& data, const string& fileName);
void NumToFile(const int num, const string& fileName);
void FileToNum(int& num, const string& fileName);
void LongLongIntToFile(const long long int num, const string& fileName);
void FileToLongLongInt(long long int& num, const string& fileName);

int SumAbs0123Diff(const vector<char>& acgtContx, const vector<char>& acgtConty);
void Cont0123(const string& s, vector<char>& cont0123);
vector<vector<char> > Cont0123(const vector<string>& vec);
string MakeStrand0123(const unsigned length, mt19937& generator);

#endif /* UTILS_HPP_ */
