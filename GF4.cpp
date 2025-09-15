/**
 * @file GF4.cpp
 * @brief Implementation of arithmetic operations and a polynomial class over GF(4).
 */

#include "GF4.hpp" // Use the new documented header
#include <cassert>
#include <iostream>
#include <random>
#include <algorithm> // For std::min

// --- Basic Arithmetic Implementations ---

// See GF4.hpp for function documentation.
int AddGF4(const int a, const int b)
{
	assert((0 <= a) && (a < 4) && (0 <= b) && (b < 4));
	// Pre-computed addition table for GF(4) based on x^2+x+1
	static const int addition[4][4] = {{0, 1, 2, 3},
									   {1, 0, 3, 2},
									   {2, 3, 0, 1},
									   {3, 2, 1, 0}};
	return addition[a][b];
}

// See GF4.hpp for function documentation.
int MulGF4(const int a, const int b)
{
	assert((0 <= a) && (a < 4) && (0 <= b) && (b < 4));
	// Pre-computed multiplication table for GF(4) based on x^2+x+1
	static const int mul[4][4] = {{0, 0, 0, 0},
								  {0, 1, 2, 3},
								  {0, 2, 3, 1},
								  {0, 3, 1, 2}};
	return mul[a][b];
}

// See GF4.hpp for function documentation.
int DivGF4(const int a, const int b)
{
	assert((0 <= a) && (a < 4) && (0 < b) && (b < 4));
	// Pre-computed division table for GF(4)
	static const int div[4][4] = {{0, 0, 0, 0},
								  {0, 1, 3, 2},
								  {0, 2, 1, 3},
								  {0, 3, 2, 1}};
	return div[a][b];
}

// --- Matrix Operation Implementation ---

// See GF4.hpp for function documentation.
vector<int> MatMulGF4(const vector<int> &v, const vector<vector<int>> &M, const int k, const int n)
{
	assert(not M.empty());
	assert((int)M.size() == k);
	assert((int)M[0].size() == n);
	vector<int> result(n);
	for (int j = 0; j < n; j++)
	{
		int res = 0;
		for (int i = 0; i < k; i++)
		{
			res = AddGF4(res, MulGF4(v[i], M[i][j]));
		}
		result[j] = res;
	}
	return result;
}

// --- PolyGF4 Class Method Implementations ---

PolyGF4::PolyGF4() : coefs(1), deg(0) {}

PolyGF4::PolyGF4(const int deg) : coefs(deg + 1, 0), deg(deg) {}

PolyGF4::PolyGF4(const vector<int> &vec) : coefs(vec), deg(vec.size() - 1)
{
	ReduceDeg();
}

void PolyGF4::ReduceDeg()
{
	while (coefs.size() > 1 && coefs.back() == 0)
	{
		coefs.pop_back();
	}
	deg = coefs.size() - 1;
}

bool PolyGF4::IsZero() const
{
	return deg == 0 && coefs[0] == 0;
}

bool PolyGF4::operator==(const PolyGF4 other) const
{
	return (deg == other.deg) && (coefs == other.coefs);
}
bool PolyGF4::operator!=(const PolyGF4 other) const
{
	return !((*this) == other);
}

vector<int> PolyGF4::Coefs() const
{
	return coefs;
}

PolyGF4 operator+(const PolyGF4 &lhs, const PolyGF4 &rhs)
{
	PolyGF4 result;
	if (lhs.deg <= rhs.deg)
	{
		result = rhs;
		for (int xpow = 0; xpow < lhs.deg + 1; xpow++)
		{
			result.coefs[xpow] = AddGF4(lhs.coefs[xpow], rhs.coefs[xpow]);
		}
	}
	else
	{ // lhs.deg > rhs.deg
		result = lhs;
		for (int xpow = 0; xpow < rhs.deg + 1; xpow++)
		{
			result.coefs[xpow] = AddGF4(lhs.coefs[xpow], rhs.coefs[xpow]);
		}
	}
	result.ReduceDeg();
	return result;
}

PolyGF4 operator*(const PolyGF4 &lhs, const PolyGF4 &rhs)
{
	int resDeg = lhs.deg + rhs.deg;
	PolyGF4 result(resDeg);
	for (int xpowl = 0; xpowl < lhs.deg + 1; xpowl++)
	{
		for (int xpowr = 0; xpowr < rhs.deg + 1; xpowr++)
		{
			int mul = MulGF4(lhs.coefs[xpowl], rhs.coefs[xpowr]);
			result.coefs[xpowl + xpowr] = AddGF4(result.coefs[xpowl + xpowr], mul);
		}
	}
	result.ReduceDeg();
	return result;
}

void Div(const PolyGF4 &lhs, const PolyGF4 &rhs, PolyGF4 &quotient, PolyGF4 &remainder)
{
	if (lhs.deg < rhs.deg)
	{
		quotient = PolyGF4(0);
		remainder = lhs;
		return;
	}

	PolyGF4 rem = lhs;
	int qdeg = lhs.deg - rhs.deg;
	PolyGF4 q(qdeg);
	assert(rhs.Coefs().back() != 0);
	while (rem.deg >= rhs.deg && !rem.IsZero())
	{
		int currdeg = rem.deg - rhs.deg;
		PolyGF4 tempq(currdeg);
		assert((rem.Coefs().back() != 0) || (rem.deg == 0));
		int qc = DivGF4(rem.Coefs().back(), rhs.Coefs().back());
		q.coefs[currdeg] = qc;
		tempq.coefs[currdeg] = qc;

		rem = tempq * rhs + rem;

		if (rem.IsZero())
			break;
	}
	quotient = q;
	remainder = rem;
}

void PolyGF4::Print() const
{
	assert((coefs.back() != 0) || (deg == 0));
	if (deg > 0)
	{
		if (coefs.back() != 1)
		{
			cout << coefs.back();
		}
		cout << "x^" << deg;
	}
	for (int currdeg = deg - 1; currdeg > 0; currdeg--)
	{
		if (coefs[currdeg] != 0)
		{
			cout << "+";
			if (coefs[currdeg] != 1)
				cout << coefs[currdeg];
			cout << "x^" << currdeg;
		}
	}
	// print constant
	if (coefs[0] != 0)
	{
		if (deg > 0)
			cout << "+";
		cout << coefs[0];
	}
	else
	{ // coefs[0]==0
		if (deg == 0)
			cout << 0;
	}
	cout << endl;
}

// --- Test Functions (Example Usage) ---

vector<int> RandPol(const int deg, mt19937 &generator)
{
	uniform_int_distribution<int> firstCoef(1, 3);
	uniform_int_distribution<int> coef(0, 3);
	vector<int> res;
	for (int i = 0; i < deg; i++)
	{
		res.push_back(coef(generator));
	}
	if (deg > 0)
		res.push_back(firstCoef(generator));
	else
		res.push_back(coef(generator));
	return res;
}

void TestAddSub(const int testNum, const int maxDeg, mt19937 &generator)
{
	uniform_int_distribution<int> degDist(0, maxDeg);
	for (int i = 0; i < testNum; i++)
	{
		int deg1 = degDist(generator);
		int deg2 = degDist(generator);
		vector<int> a = RandPol(deg1, generator);
		vector<int> b = RandPol(deg2, generator);
		PolyGF4 A(a), B(b);
		PolyGF4 Sum = A + B;
		PolyGF4 D1 = Sum + B;
		PolyGF4 D2 = Sum + A;
		if (D1 != A || D2 != B)
		{
			cout << "Polynomial addition test FAILURE" << endl;
			return;
		}
	}
	cout << "Polynomial addition test SUCCESS" << endl;
}

void TestMulDiv(const int testNum, const int maxDeg, mt19937 &generator)
{
	uniform_int_distribution<int> degDist(0, maxDeg);
	for (int i = 0; i < testNum; i++)
	{
		int deg1 = degDist(generator);
		int deg2 = degDist(generator);
		vector<int> a = RandPol(deg1, generator);
		vector<int> b = RandPol(deg2, generator);
		PolyGF4 A(a), B(b);
		if (A.IsZero() || B.IsZero())
			continue;
		PolyGF4 mul = A * B;
		PolyGF4 q1, r1, q2, r2;
		Div(mul, A, q1, r1);
		Div(mul, B, q2, r2);
		if (q1 != B || q2 != A || (!r1.IsZero()) || (!r2.IsZero()))
		{
			cout << "Polynomial multiplication test FAILURE" << endl;
			return;
		}
	}
	cout << "Polynomial multiplication test SUCCESS" << endl;
}

void TestDivRem(const int testNum, const int maxDeg, mt19937 &generator)
{
	uniform_int_distribution<int> degDist1(1, maxDeg);
	for (int i = 0; i < testNum; i++)
	{
		int deg1 = degDist1(generator);
		int deg2 = degDist1(generator);
		int degr = std::min(deg1, deg2) - 1;
		assert(degr >= 0);
		vector<int> a = RandPol(deg1, generator);
		vector<int> b = RandPol(deg2, generator);
		vector<int> r = RandPol(degr, generator);
		PolyGF4 A(a), B(b), R(r);
		if (A.IsZero() || B.IsZero())
			continue;
		PolyGF4 mul = A * B + R;
		PolyGF4 q1, r1;
		Div(mul, B, q1, r1);
		if (q1 != A || r1 != R)
		{
			cout << "Polynomial division remainder test FAILURE" << endl;
			return;
		}
	}
	cout << "Polynomial division remainder test SUCCESS" << endl;
}
