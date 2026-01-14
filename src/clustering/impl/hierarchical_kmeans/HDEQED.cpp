#include <iostream>
#include <vector>
#include <cassert>
#include <map>
#include <climits>
#include "pipeline_utils.hpp"
	
using namespace std;

namespace indexgen {
namespace clustering {
namespace impl {


int FastEditDistance_here(const string &source, const string &target)
{
	if (source.size() > target.size())
	{
		return FastEditDistance_here(target, source);
	}

	const int min_size = source.size(), max_size = target.size();
	vector<int> lev_dist(min_size + 1);

	for (int i = 0; i <= min_size; ++i)
	{
		lev_dist[i] = i;
	}

	for (int j = 1; j <= max_size; ++j)
	{
		int previous_diagonal = lev_dist[0], previous_diagonal_save;
		++lev_dist[0];

		for (int i = 1; i <= min_size; ++i)
		{
			previous_diagonal_save = lev_dist[i];
			if (source[i - 1] == target[j - 1])
			{
				lev_dist[i] = previous_diagonal;
			}
			else
			{
				lev_dist[i] = min(min(lev_dist[i - 1], lev_dist[i]), previous_diagonal) + 1;
			}
			previous_diagonal = previous_diagonal_save;
		}
	}

	return lev_dist[min_size];
}

int HammingDist(const string &str1, const string &str2)
{
	assert(str1.size() == str2.size());
	int count = 0;
	for (int i = 0; i < (int)str1.size(); i++)
	{
		if (str1[i] != str2[i])
			count++;
	}
	return count;
}

int ArgIndexMinSumED(const vector<vector<int>> &edMat)
{
	int N = edMat.size();
	int minSumED = INT_MAX;
	int argIndex = 0;
	for (int i = 0; i < N; i++)
	{
		int sumED = 0;
		const vector<int> &currVec = edMat[i];
		for (int j = 0; j < N; j++)
		{
			sumED += currVec[j];
			if (sumED > minSumED)
				break;
		}
		if (sumED < minSumED)
		{
			minSumED = sumED;
			argIndex = i;
		}
	}
	return argIndex;
}

// return index of s in strs2 st. sumED(s,strs1) is minimal
int ArgIndexMinSumED(const vector<string> &strs1, const vector<string> &strs2)
{
	long long minSumED = LLONG_MAX;
	int argIndex = 0;
	for (int i = 0; i < (int)strs2.size(); i++)
	{
		int sumED = 0;
		for (const string &s : strs1)
		{
			int ed = FastEditDistance_here(strs2[i], s);
			sumED += ed;
			if (sumED > minSumED)
				break;
		}
		if (sumED < minSumED)
		{
			minSumED = sumED;
			argIndex = i;
		}
	}
	return argIndex;
}

void EDMatrix(vector<vector<int>> &edMat, const vector<string> &strs)
{
	int N = strs.size();
	edMat = vector<vector<int>>(N, vector<int>(N));
	for (int i = 0; i < N; i++)
	{
		for (int j = i + 1; j < N; j++)
		{
			int ed = FastEditDistance_here(strs[i], strs[j]);
			edMat[i][j] = ed;
			edMat[j][i] = ed;
		}
	}
}

vector<int> HDEQEDStrsPtrs(const int index, const vector<string> &strs, const int designLen,
						   const vector<vector<int>> &edMat)
{
	vector<int> res;
	for (unsigned i = 0; i < strs.size(); i++)
	{
		if ((int)strs[i].size() != designLen)
		{
			cout << "String at index " << i << " has incorrect length: " << strs[i].size() << endl;
			cout << "Expected length: " << designLen << endl;
			cout << "the string is: " << strs[i] << endl;
		}
		assert((int)strs[i].size() == designLen);
		int hamm = HammingDist(strs[index], strs[i]);
		if (hamm == edMat[index][i])
		{
			res.push_back(i);
		}
	}
	return res;
}

char MaxChar(const map<char, int> &count)
{
	assert(!count.empty());
	char maxChar = count.begin()->first;
	int maxCount = count.begin()->second;
	for (const pair<char, int> &pr : count)
	{
		if (pr.second > maxCount)
		{
			maxCount = pr.second;
			maxChar = pr.first;
		}
	}
	return maxChar;
}

string MajStringPtrs(const vector<string> &strs, const vector<int> &ptrs)
{
	assert(!strs.empty());
	const int n = strs[0].size();
	vector<map<char, int>> count(n);
	for (unsigned i = 0; i < ptrs.size(); i++)
	{
		const string &currStr = strs[ptrs[i]];
		assert((int)currStr.size() == n);
		for (int pos = 0; pos < n; pos++)
		{
			count[pos][currStr[pos]]++;
		}
	}
	string res(n, '0');
	for (int i = 0; i < n; i++)
	{
		res[i] = MaxChar(count[i]);
	}
	return res;
}

string HDEQEDSimpleMat(const vector<string> &cluster, int index, const int designLen, const vector<vector<int>> &edMat)
{
	vector<int> hdeqedStrandsPtrs = HDEQEDStrsPtrs(index, cluster, designLen, edMat);
	return MajStringPtrs(cluster, hdeqedStrandsPtrs);
}

void CorrectClusterHDEQEDSimpleFast(const vector<string> &cluster, vector<string> &correctedCluster, const int designLen,
									const vector<vector<int>> &edMat)
{
	int clusterSize = cluster.size();
	correctedCluster.clear();
	correctedCluster.resize(clusterSize);
	for (int copyIndex = 0; copyIndex < clusterSize; copyIndex++)
	{
		correctedCluster[copyIndex] = HDEQEDSimpleMat(cluster, copyIndex, designLen, edMat);
	}
}

/* HDEQED - Fast alternative to CPL
 * cluster - vector of strings of EQUAL length designLen
 * return - a string s, s.t. the sum of its edit distances from the strings in the cluster is as small as possible.
 * NOTE: algorithm designed for relatively large clusters (tested on sizes: 200, 500, 1000) and relatively short strings (tested on lengths: 14, 20)
 * May not work in other contexts. Use CPL instead.
 * */

// 50 times faster than CPL. Sum of edit distances from cluster strings slightly larger than that of CPL
string HDEQEDFixMinSumFast(const vector<string> &cluster, const int designLen)
{
	vector<vector<int>> edMat;
	EDMatrix(edMat, cluster);
	int index = ArgIndexMinSumED(edMat);
	return HDEQEDSimpleMat(cluster, index, designLen, edMat);
}

// 15 times faster than CPL. Sum of edit distances from cluster strings slightly smaller than that of CPL
string HDEQEDMinSumOfCorrectedClusterFast(const vector<string> &cluster, const int designLen)
{
	vector<string> correctedCluster;
	vector<vector<int>> edMat;
	EDMatrix(edMat, cluster);
	CorrectClusterHDEQEDSimpleFast(cluster, correctedCluster, designLen, edMat);
	int index = ArgIndexMinSumED(cluster, correctedCluster);
	return correctedCluster[index];
}

} // namespace impl
} // namespace clustering
} // namespace indexgen

