/**
 * @file GF4.hpp
 * @brief Defines arithmetic operations and a polynomial class over the Galois Field of 4 elements (GF(4)).
 *
 * This file provides the fundamental mathematical tools for linear block codes. The four
 * elements {0, 1, 2, 3} can be used to represent the four DNA bases (e.g., A=0, C=1, G=2, T=3).
 * All arithmetic is performed modulo an irreducible polynomial (x^2 + x + 1), which defines
 * the field's structure.
 */

#ifndef GF4_HPP_
#define GF4_HPP_

#include <vector>
#include <iostream>

// Using `std` for convenience.
using namespace std;

// --- Basic Arithmetic Operations ---

/**
 * @brief Performs addition over GF(4).
 * @param a An element in {0, 1, 2, 3}.
 * @param b An element in {0, 1, 2, 3}.
 * @return The sum (a + b) in GF(4).
 */
int AddGF4(const int a, const int b);

/**
 * @brief Performs multiplication over GF(4).
 * @param a An element in {0, 1, 2, 3}.
 * @param b An element in {0, 1, 2, 3}.
 * @return The product (a * b) in GF(4).
 */
int MulGF4(const int a, const int b);

/**
 * @brief Performs division over GF(4).
 * @param a The numerator, an element in {0, 1, 2, 3}.
 * @param b The denominator, a non-zero element in {1, 2, 3}.
 * @return The quotient (a / b) in GF(4).
 */
int DivGF4(const int a, const int b);

// --- Matrix Operation ---

/**
 * @brief Multiplies a vector by a matrix over GF(4).
 * @details Performs the operation `v * M` where `v` is a row vector.
 * @param v The row vector of size 1 x k.
 * @param M The matrix of size k x n.
 * @param k The number of columns in v / rows in M.
 * @param n The number of columns in M.
 * @return The resulting row vector of size 1 x n.
 */
vector<int> MatMulGF4(const vector<int> &v, const vector<vector<int>> &M, const int k, const int n);

// --- Polynomial Class over GF(4) ---

/**
 * @class PolyGF4
 * @brief Represents a polynomial with coefficients in GF(4).
 * @details This class supports basic polynomial arithmetic (addition, multiplication, division)
 * required for more advanced coding theory constructs like BCH or Reed-Solomon codes.
 * The polynomial is stored as a vector of coefficients.
 */
class PolyGF4
{
private:
    vector<int> coefs; // coefs[i] is the coefficient of x^i
    int deg;           // The degree of the polynomial

public:
    // --- Constructors ---
    PolyGF4();
    PolyGF4(const int deg);
    PolyGF4(const vector<int> &vec);

    // --- Utility Methods ---
    void ReduceDeg();
    bool IsZero() const;
    vector<int> Coefs() const;
    void Print() const;

    // --- Overloaded Operators ---
    bool operator==(const PolyGF4 other) const;
    bool operator!=(const PolyGF4 other) const;
    friend PolyGF4 operator+(const PolyGF4 &lhs, const PolyGF4 &rhs);
    friend PolyGF4 operator*(const PolyGF4 &lhs, const PolyGF4 &rhs);

    /**
     * @brief Performs polynomial division.
     * @details Calculates `lhs / rhs`, producing a quotient and a remainder.
     * @param lhs The dividend polynomial.
     * @param rhs The divisor polynomial.
     * @param quotient The resulting quotient polynomial (output).
     * @param remainder The resulting remainder polynomial (output).
     */
    friend void Div(const PolyGF4 &lhs, const PolyGF4 &rhs, PolyGF4 &quotient, PolyGF4 &remainder);
};

#endif /* GF4_HPP_ */
