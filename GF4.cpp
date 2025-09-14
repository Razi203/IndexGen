#include <cassert>
#include <iostream>
#include <random>
#include "GF4.hpp"

int AddGF4(const int a, const int b) {
	assert((0 <= a) && (a < 4) && (0 <= b) && (b < 4));
	static const int addition[4][4] = { { 0, 1, 2, 3 },
										{ 1, 0, 3, 2 },
										{ 2, 3, 0, 1 },
										{ 3, 2, 1, 0 } };
	return addition[a][b];
}

int MulGF4(const int a, const int b) {
	assert((0 <= a) && (a < 4) && (0 <= b) && (b < 4));
	static const int mul[4][4] = {	{ 0, 0, 0, 0 },
									{ 0, 1, 2, 3 },
									{ 0, 2, 3, 1 },
									{ 0, 3, 1, 2 } };
	return mul[a][b];
}

// a/b
int DivGF4(const int a, const int b) {
	assert((0 <= a) && (a < 4) && (0 < b) && (b < 4));
	static const int div[4][4] = {	{ 0, 0, 0, 0 },
									{ 0, 1, 3, 2 },
									{ 0, 2, 1, 3 },
									{ 0, 3, 2, 1 } };
	return div[a][b];
}

// GF4 multiplication v(1xk) M(kxn)
vector<int> MatMulGF4(const vector<int>& v, const vector<vector<int> >& M, const int k, const int n) {
	assert(not M.empty());
	assert((int ) M.size() == k);
	assert((int ) M[0].size() == n);
	vector<int> result(n);
	for (int j = 0; j < n; j++) {
		int res = 0;
		for (int i = 0; i < k; i++) {
			res = AddGF4(res, MulGF4(v[i], M[i][j]));
		}
		result[j] = res;
	}
	return result;
}

class PolyGF4 {
	// vector of coefficients of polynomial: coefs[i]= coefficient of x^i
	vector<int> coefs;
	int deg;
public:
	PolyGF4():coefs(1),deg(0){

	}
	PolyGF4(const int deg) :
			coefs(deg + 1), deg(deg) {

	}
	PolyGF4(const vector<int>& vec):coefs(vec),deg(vec.size()-1){
		ReduceDeg();
	}
	void ReduceDeg() {
		while (coefs.size() > 1 and coefs.back() == 0) {
			coefs.pop_back();
		}
		deg = coefs.size() - 1;
	}

	bool IsZero() const {
		return deg==0 and coefs[0] == 0;
	}

	bool operator==(const PolyGF4 other) const {
		return (deg == other.deg) and (coefs == other.coefs);
	}
	bool operator!=(const PolyGF4 other) const {
		return !((*this) == other);
	}

	vector<int> Coefs() const{
		return coefs;
	}

	friend PolyGF4 operator+(const PolyGF4& lhs, const PolyGF4& rhs) {
		PolyGF4 result;
		if (lhs.deg <= rhs.deg) {
			result = rhs;
			for (int xpow = 0; xpow < lhs.deg + 1; xpow++) {
				result.coefs[xpow] = AddGF4(lhs.coefs[xpow], rhs.coefs[xpow]);
			}
		}
		else { // lhs.deg > rhs.deg
			result = lhs;
			for (int xpow = 0; xpow < rhs.deg + 1; xpow++) {
				result.coefs[xpow] = AddGF4(lhs.coefs[xpow], rhs.coefs[xpow]);
			}

		}
		result.ReduceDeg();
		return result;
	}

	friend PolyGF4 operator*(const PolyGF4& lhs, const PolyGF4& rhs) {
		int resDeg = lhs.deg + rhs.deg;
		PolyGF4 result(resDeg);
		for (int xpowl = 0; xpowl < lhs.deg + 1; xpowl++) {
			for (int xpowr = 0; xpowr < rhs.deg + 1; xpowr++) {
				int mul = MulGF4(lhs.coefs[xpowl], rhs.coefs[xpowr]);
				result.coefs[xpowl + xpowr] = AddGF4(result.coefs[xpowl + xpowr], mul);
			}
		}
		result.ReduceDeg();
		return result;
	}

	friend void Div(const PolyGF4& lhs, const PolyGF4& rhs, PolyGF4& quotient, PolyGF4& remainder) {
		if (lhs.deg < rhs.deg) {
			quotient = PolyGF4(0);
			remainder = lhs;
			return;
		}

		PolyGF4 rem = lhs;
		int qdeg = lhs.deg - rhs.deg;
		PolyGF4 q(qdeg);
		assert(rhs.coefs.back() != 0);
		while (rem.deg >= rhs.deg) {
			int currdeg = rem.deg - rhs.deg;
			PolyGF4 tempq(currdeg);
			assert((rem.coefs.back() != 0) or (rem.deg == 0));
			int qc = DivGF4(rem.coefs.back(), rhs.coefs.back());
			q.coefs[currdeg] = qc;
			tempq.coefs[currdeg] = qc;
			int oldremdeg = rem.deg;
			rem = tempq * rhs + rem;
			int newremdeg = rem.deg;
			if (rem.IsZero())
				break;
			assert(oldremdeg > newremdeg);
		}
		quotient = q;
		remainder = rem;
	}
	void Print() const {
		assert((coefs.back() != 0) or (deg == 0));
		if (deg > 0) {
			if (coefs.back() != 1) {
				cout << coefs.back();
			}
			cout << "x^" << deg;
		}
		for (int currdeg = deg - 1; currdeg > 0; currdeg--) {
			if (coefs[currdeg] != 0) {
				cout << "+";
				if (coefs[currdeg] != 1)
					cout << coefs[currdeg];
				cout << "x^" << currdeg;
			}
		}
		// print constant
		if (coefs[0] != 0) {
			if (deg > 0)
				cout << "+";
			cout << coefs[0];
		}
		else { //coefs[0]==0
			if (deg == 0)
				cout << 0;
		}
		cout << endl;
	}
};

void TestPoly1() {
	vector<int> a = { 1, 1, 1 };
	vector<int> b = { 1, 1 };
	vector<int> one = { 1 };
	PolyGF4 p1(a), p2(b);
	PolyGF4 mul = p1 * p2 + one;
	PolyGF4 q, r;
	Div(mul, p2, q, r);
	q.Print();
	r.Print();
}

vector<int> RandPol(const int deg, mt19937& generator) {
	uniform_int_distribution<int> firstCoef(1, 3);
	uniform_int_distribution<int> coef(0, 3);
	vector<int> res;
	for (int i = 0; i < deg; i++) {
		res.push_back(coef(generator));
	}
	if (deg > 0)
		res.push_back(firstCoef(generator));
	else
		res.push_back(coef(generator));
	return res;
}

void TestAddSub(const int testNum, const int maxDeg, mt19937& generator) {
	uniform_int_distribution<int> degDist(0, maxDeg);
	for (int i = 0; i < testNum; i++) {
		int deg1 = degDist(generator);
		int deg2 = degDist(generator);
		vector<int> a = RandPol(deg1,generator);
		vector<int> b = RandPol(deg2,generator);
		PolyGF4 A(a), B(b);
		PolyGF4 Sum = A+B;
		PolyGF4 D1 = Sum + B;
		PolyGF4 D2 = Sum + A;
		if (D1 != A or D2!=B){
			cout << "Polynomial addition test FAILURE" << endl;
			return;
		}
	}
	cout << "Polynomial addition test SUCCESS" << endl;
}

void TestMulDiv(const int testNum, const int maxDeg, mt19937& generator) {
	uniform_int_distribution<int> degDist(0, maxDeg);
	for (int i = 0; i < testNum; i++) {
		int deg1 = degDist(generator);
		int deg2 = degDist(generator);
		vector<int> a = RandPol(deg1, generator);
		vector<int> b = RandPol(deg2, generator);
		PolyGF4 A(a), B(b);
		if (A.IsZero() or B.IsZero())
			continue;
		PolyGF4 mul = A * B;
		PolyGF4 q1, r1, q2, r2;
		Div(mul, A, q1, r1);
		Div(mul, B, q2, r2);
		if (q1 != B or q2 != A or (!r1.IsZero()) or (!r2.IsZero())) {
			cout << "Polynomial multiplication test FAILURE" << endl;
			return;
		}
	}
	cout << "Polynomial multiplication test SUCCESS" << endl;
}

void TestDivRem(const int testNum, const int maxDeg, mt19937& generator) {
	uniform_int_distribution<int> degDist1(1, maxDeg);
	for (int i = 0; i < testNum; i++) {
		int deg1 = degDist1(generator);
		int deg2 = degDist1(generator);
		int degr = min(deg1, deg2) - 1;
		assert(degr>=0);
		vector<int> a = RandPol(deg1, generator);
		vector<int> b = RandPol(deg2, generator);
		vector<int> r = RandPol(degr, generator);
		PolyGF4 A(a), B(b), R(r);
		if (A.IsZero() or B.IsZero())
			continue;
		PolyGF4 mul = A * B + R;
		PolyGF4 q1, r1, q2, r2;
		Div(mul, A, q1, r1);
		Div(mul, B, q2, r2);
		if (q1 != B or q2 != A or r1 != R or r2 != R) {
			cout << "Polynomial division remainder test FAILURE" << endl;
			cout << "A: ";
			A.Print();
			cout << "B: ";
			B.Print();
			cout << "R: ";
			R.Print();
			cout << "A*B+R: ";
			mul.Print();

			return;
		}
	}
	cout << "Polynomial division remainder test SUCCESS" << endl;
}
