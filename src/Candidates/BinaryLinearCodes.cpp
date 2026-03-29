/**
 * @file BinaryLinearCodes.cpp
 * @brief Implementation for generating binary linear codes over GF(2).
 *
 * Supports Hamming distances 2-7:
 * - d=2: Simple parity-check code
 * - d=3: Hamming code (algorithmic H-matrix construction)
 * - d=4: Extended Hamming code
 * - d=5: BCH code with g(x) = lcm(M1, M3)
 * - d=6: Extended BCH (d=5 BCH + overall parity)
 * - d=7: BCH code with g(x) = lcm(M1, M3, M5)
 */

#include "Candidates/BinaryLinearCodes.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>

// ============================================================================
// GF(2) Arithmetic (trivial: XOR and AND)
// ============================================================================

/**
 * @brief Matrix-vector multiplication over GF(2): result = v * M
 * @param v Row vector (1 x k)
 * @param M Matrix (k x n)
 * @param k Number of rows in M
 * @param n Number of columns in M
 * @return Result vector (1 x n)
 */
static vector<int> MatMulGF2(const vector<int> &v, const vector<vector<int>> &M, const int k, const int n)
{
    assert(!M.empty());
    assert((int)M.size() == k);
    assert((int)M[0].size() == n);
    vector<int> result(n, 0);
    for (int j = 0; j < n; j++)
    {
        int res = 0;
        for (int i = 0; i < k; i++)
        {
            res ^= (v[i] & M[i][j]); // GF(2): multiply = AND, add = XOR
        }
        result[j] = res;
    }
    return result;
}

// ============================================================================
// Permutation helpers (reused from LinearCodes pattern)
// ============================================================================

static vector<vector<int>> BinaryPermuteColumns(const vector<vector<int>> &mat, const vector<int> &perm)
{
    if (perm.empty())
        return mat;

    int rowNum = mat.size();
    int colNum = mat[0].size();

    vector<vector<int>> permutedMat(rowNum, vector<int>(colNum));
    for (int i = 0; i < rowNum; i++)
    {
        for (int j = 0; j < colNum; j++)
        {
            permutedMat[i][j] = mat[i][perm[j]];
        }
    }
    return permutedMat;
}

static vector<vector<int>> BinaryPermuteRows(const vector<vector<int>> &mat, const vector<int> &perm)
{
    if (perm.empty())
        return mat;

    int rowNum = mat.size();
    int colNum = mat[0].size();

    vector<vector<int>> permutedMat(rowNum, vector<int>(colNum));
    for (int i = 0; i < rowNum; i++)
    {
        for (int j = 0; j < colNum; j++)
        {
            permutedMat[i][j] = mat[perm[i]][j];
        }
    }
    return permutedMat;
}

// ============================================================================
// Shortening: remove first delNum rows and columns from generator matrix
// ============================================================================

static vector<vector<int>> BinaryShorten(const vector<vector<int>> &mat, const int delNum)
{
    if (delNum == 0)
        return mat;
    assert(delNum > 0);
    assert(!mat.empty());
    int rowNum = mat.size();
    int colNum = mat[0].size();

    // Shortening a systematic code [I_k | P] by delNum positions
    // means keeping codewords that have 0 in first delNum positions.
    // In G, these are the rows delNum...rowNum-1.
    // Then we drop the first delNum columns (which are all 0 in these rows).
    assert(rowNum > delNum);
    assert(colNum > delNum);

    vector<vector<int>> result;
    for (int row = delNum; row < rowNum; row++)
    {
        vector<int> currRow(mat[row].begin() + delNum, mat[row].end());
        result.push_back(currRow);
    }
    return result;
}

// ============================================================================
// Generator Matrix Construction
// ============================================================================

/**
 * @brief Creates a generator matrix for a binary parity-check code with Hamming distance 2.
 * G = [I_{n-1} | 1-column], producing an [n, n-1, 2] code.
 */
static vector<vector<int>> BinaryGenMat2(const int n)
{
    assert(n >= 2);
    vector<vector<int>> genMat(n - 1, vector<int>(n, 0));
    // (n-1) x (n-1) identity matrix
    for (int i = 0; i < n - 1; i++)
        genMat[i][i] = 1;
    // Last column: all 1's (parity check)
    for (int i = 0; i < n - 1; i++)
        genMat[i][n - 1] = 1;
    return genMat;
}

/**
 * @brief Constructs a Hamming code parity-check matrix H of size r x (2^r - 1).
 * Columns are all non-zero binary vectors of length r in ascending order.
 */
static vector<vector<int>> BuildHammingH(int r)
{
    int n = (1 << r) - 1; // 2^r - 1
    vector<vector<int>> H(r, vector<int>(n, 0));

    for (int col = 0; col < n; col++)
    {
        int val = col + 1; // 1 to 2^r - 1 (all non-zero r-bit vectors)
        for (int row = r - 1; row >= 0; row--)
        {
            H[row][col] = (val >> (r - 1 - row)) & 1;
        }
    }
    return H;
}

/**
 * @brief Row-reduce a binary matrix to RREF over GF(2) and extract the systematic generator matrix.
 * Given an H matrix of size r x n, constructs G = [I_k | P] such that G * H^T = 0.
 *
 * @param H The parity-check matrix (r x n).
 * @param r Number of parity bits.
 * @param n Code length.
 * @return The generator matrix G of size k x n in [I_k | P] form.
 */
static vector<vector<int>> HToSystematicG(vector<vector<int>> H, int r, int n)
{
    int k = n - r;

    // Put H in form [P | I_r] via row reduction and column permutations
    vector<vector<int>> Hwork = H;

    // Identity rows targets: we want identity at the END of H (last r columns)
    for (int pivotRow = 0; pivotRow < r; pivotRow++)
    {
        int targetCol = k + pivotRow;
        // Find a column to become the pivot
        int pivotCol = -1;
        for (int col = 0; col < n; col++)
        {
            // Check if this column is already used as a pivot or is the target column
            // For simplicity, we just find the first available column that has a 1
            if (Hwork[pivotRow][col] == 1)
            {
                // Verify this column doesn't have 1s in rows we already processed
                bool usable = true;
                for (int prevRow = 0; prevRow < pivotRow; prevRow++)
                    if (Hwork[prevRow][col] == 1) usable = false;
                
                if (usable) {
                    pivotCol = col;
                    break;
                }
            }
        }
        
        if (pivotCol == -1) // Should not happen for Hamming codes correctly defined
        {
            // Fallback: just find ANY 1 in this row in any remaining column
            for (int col = 0; col < n; col++) {
                if (Hwork[pivotRow][col] == 1) { pivotCol = col; break; }
            }
        }

        assert(pivotCol != -1);

        // Swap columns to put pivot at targetCol
        if (pivotCol != targetCol)
        {
            for (int row = 0; row < r; row++)
                std::swap(Hwork[row][pivotCol], Hwork[row][targetCol]);
        }

        // Eliminate
        for (int row = 0; row < r; row++)
        {
            if (row != pivotRow && Hwork[row][targetCol] == 1)
            {
                for (int col = 0; col < n; col++)
                    Hwork[row][col] ^= Hwork[pivotRow][col];
            }
        }
    }

    // Now Hwork is NOT necessarily [P | I_r] because row operations
    // might have put identity columns in different rows.
    // Let's ensure Hwork is [P | I_r] by reordering rows
    vector<vector<int>> Hfinal(r, vector<int>(n));
    for (int i = 0; i < r; i++) {
        // Find which row has identity bit in column k+i
        for (int row = 0; row < r; row++) {
            if (Hwork[row][k+i] == 1) {
                Hfinal[i] = Hwork[row];
                break;
            }
        }
    }

    // Extract P (r x k)
    vector<vector<int>> P(r, vector<int>(k));
    for (int i = 0; i < r; i++)
        for (int j = 0; j < k; j++)
            P[i][j] = Hfinal[i][j];

    // G = [I_k | P^T]
    vector<vector<int>> G(k, vector<int>(n, 0));
    for (int i = 0; i < k; i++)
    {
        G[i][i] = 1;
        for (int j = 0; j < r; j++)
            G[i][k + j] = P[j][i];
    }

    return G;
}

/**
 * @brief Creates a generator matrix for a Hamming code with distance 3.
 * The Hamming code has parameters [2^r - 1, 2^r - r - 1, 3].
 * For n < 2^r - 1, the code is shortened.
 */
static vector<vector<int>> BinaryGenMat3(const int n)
{
    assert(n >= 3);
    // Find smallest r such that 2^r - 1 >= n
    int r = 1;
    while ((1 << r) - 1 < n)
        r++;

    int n_hamming = (1 << r) - 1;

    // Build the full Hamming parity-check matrix
    vector<vector<int>> H = BuildHammingH(r);

    // Convert to systematic generator matrix
    vector<vector<int>> G = HToSystematicG(H, r, n_hamming);

    // Shorten if needed
    if (n_hamming > n)
    {
        G = BinaryShorten(G, n_hamming - n);
    }

    return G;
}

/**
 * @brief Creates a generator matrix for an Extended Hamming code with distance 4.
 * Takes a Hamming [2^r-1, 2^r-r-1, 3] code and appends an overall parity bit.
 */
static vector<vector<int>> BinaryGenMat4(const int n)
{
    assert(n >= 4);
    // Build a distance-3 code of length n-1
    vector<vector<int>> G3 = BinaryGenMat3(n - 1);
    int k = G3.size();

    // Extend: append overall parity column
    vector<vector<int>> G(k, vector<int>(n, 0));
    for (int i = 0; i < k; i++)
    {
        int parity = 0;
        for (int j = 0; j < n - 1; j++)
        {
            G[i][j] = G3[i][j];
            parity ^= G3[i][j];
        }
        G[i][n - 1] = parity; // Overall parity bit
    }

    return G;
}

// ============================================================================
// BCH Code Construction
// ============================================================================

/**
 * @brief Multiply two polynomials over GF(2).
 * Coefficients are stored with coef[i] = coefficient of x^i.
 */
static vector<int> PolyMulGF2(const vector<int> &a, const vector<int> &b)
{
    if (a.empty() || b.empty())
        return {};
    int da = a.size() - 1;
    int db = b.size() - 1;
    vector<int> result(da + db + 1, 0);
    for (int i = 0; i <= da; i++)
    {
        for (int j = 0; j <= db; j++)
        {
            result[i + j] ^= (a[i] & b[j]);
        }
    }
    return result;
}

/**
 * @brief Divide polynomial a by polynomial b over GF(2), return remainder.
 */
static vector<int> PolyModGF2(vector<int> a, const vector<int> &b)
{
    int da = a.size() - 1;
    int db = b.size() - 1;
    if (da < db)
        return a;

    for (int i = da; i >= db; i--)
    {
        if (a[i] == 1)
        {
            for (int j = 0; j <= db; j++)
            {
                a[i - db + j] ^= b[j];
            }
        }
    }
    // Remove leading zeros
    while (a.size() > 1 && a.back() == 0)
        a.pop_back();
    return a;
}

/**
 * @brief Compute the minimal polynomial of alpha^power in GF(2^m).
 *
 * Alpha is a root of the primitive polynomial for GF(2^m).
 * The minimal polynomial M_i(x) is the smallest polynomial over GF(2)
 * with alpha^i as a root.
 *
 * @param m The extension degree (field is GF(2^m))
 * @param power The power of alpha
 * @param prim_poly The primitive polynomial coefficients (degree m)
 * @return The minimal polynomial coefficients
 */
static vector<int> MinimalPoly(int m, int power, const vector<int> &prim_poly)
{
    int n = (1 << m) - 1; // 2^m - 1

    // Build GF(2^m) elements as integers (bit representations)
    // alpha^0 = 1, alpha^1 = 2, etc., with reduction by prim_poly
    vector<int> exp_table(n);
    exp_table[0] = 1;
    for (int i = 1; i < n; i++)
    {
        int val = exp_table[i - 1] << 1; // multiply by alpha
        if (val & (1 << m))
        { // reduce if degree >= m
            // XOR with primitive polynomial (excluding leading term)
            int pp = 0;
            for (int j = 0; j < m; j++)
                if (prim_poly[j])
                    pp |= (1 << j);
            val ^= pp;
            val &= (1 << m) - 1;
        }
        exp_table[i] = val;
    }

    // Build log table
    vector<int> log_table(1 << m, -1);
    for (int i = 0; i < n; i++)
        log_table[exp_table[i]] = i;

    // Find the conjugate set of alpha^power: {alpha^power, alpha^(2*power), alpha^(4*power), ...}
    // until we cycle back to alpha^power
    vector<int> conjugates;
    int p = power % n;
    do
    {
        conjugates.push_back(p);
        p = (2 * p) % n;
    } while (p != (power % n));

    // The minimal polynomial is the product of (x - alpha^c) for each conjugate c
    // Over GF(2), this is product of (x + alpha^c) = (x + alpha^c) since -1 = 1 in GF(2)
    vector<int> result = {1}; // Start with the polynomial "1"

    for (int c : conjugates)
    {
        // Multiply result by (x + alpha^c)
        // In our representation: [alpha^c, 1] where coeff of x^0 = alpha^c, x^1 = 1
        int alpha_c = exp_table[c % n];

        // New polynomial = result * (x + alpha^c) over GF(2^m)
        int deg = result.size();
        vector<int> new_result(deg + 1, 0);

        for (int i = 0; i < deg; i++)
        {
            // Multiply result[i] by alpha^c (field multiplication)
            if (result[i] != 0)
            {
                int prod;
                if (result[i] == 0)
                    prod = 0;
                else
                    prod = exp_table[(log_table[result[i]] + log_table[alpha_c]) % n];
                new_result[i] ^= prod;
            }
            // result[i] * x -> new_result[i+1]
            new_result[i + 1] ^= result[i];
        }

        result = new_result;
    }

    // Convert from GF(2^m) coefficients to GF(2) coefficients
    // Each coefficient should be 0 or 1 since it's a minimal polynomial over GF(2)
    vector<int> binary_result(result.size());
    for (int i = 0; i < (int)result.size(); i++)
    {
        binary_result[i] = result[i] & 1; // Should already be 0 or 1 for a valid minimal poly
    }

    return binary_result;
}

/**
 * @brief Get the primitive polynomial for GF(2^m).
 * Returns coefficients of the primitive polynomial with coef[i] = coefficient of x^i.
 */
static vector<int> GetPrimitivePoly(int m)
{
    // Well-known primitive polynomials for small m values
    // These are: x^m + lower_terms + 1
    switch (m)
    {
    case 2:
        return {1, 1, 1}; // x^2 + x + 1
    case 3:
        return {1, 1, 0, 1}; // x^3 + x + 1
    case 4:
        return {1, 1, 0, 0, 1}; // x^4 + x + 1
    case 5:
        return {1, 0, 1, 0, 0, 1}; // x^5 + x^2 + 1
    case 6:
        return {1, 1, 0, 0, 0, 0, 1}; // x^6 + x + 1
    default:
        throw std::runtime_error("Primitive polynomial not defined for m=" + std::to_string(m));
    }
}

/**
 * @brief Compute LCM of two polynomials over GF(2).
 * lcm(a, b) = (a * b) / gcd(a, b)
 */
static vector<int> PolyGCD_GF2(vector<int> a, vector<int> b)
{
    while (!b.empty() && !(b.size() == 1 && b[0] == 0))
    {
        vector<int> r = PolyModGF2(a, b);
        a = b;
        b = r;
    }
    return a;
}

static vector<int> PolyLCM_GF2(const vector<int> &a, const vector<int> &b)
{
    vector<int> g = PolyGCD_GF2(a, b);
    // lcm = a * b / gcd
    // Since we're over GF(2), division is exact
    vector<int> prod = PolyMulGF2(a, b);

    // Polynomial division of prod by g (should be exact)
    int dp = prod.size() - 1;
    int dg = g.size() - 1;
    if (dp < dg)
        return prod;

    vector<int> quotient(dp - dg + 1, 0);
    vector<int> remainder = prod;

    for (int i = dp; i >= dg; i--)
    {
        if (remainder[i] == 1)
        {
            quotient[i - dg] = 1;
            for (int j = 0; j <= dg; j++)
                remainder[i - dg + j] ^= g[j];
        }
    }

    // Remove trailing zeros
    while (quotient.size() > 1 && quotient.back() == 0)
        quotient.pop_back();

    return quotient;
}

/**
 * @brief Build a systematic generator matrix from a BCH generator polynomial.
 *
 * Given generator polynomial g(x) of degree r = n-k, constructs the (k x n) systematic
 * generator matrix G = [I_k | P] where P is derived from remainders of x^(n-k+i) mod g(x).
 *
 * @param gen_poly The generator polynomial coefficients.
 * @param n The code length.
 * @return The generator matrix.
 */
static vector<vector<int>> GenMatFromPoly(const vector<int> &gen_poly, int n)
{
    int r = gen_poly.size() - 1; // degree = n - k
    int k = n - r;
    assert(k > 0);

    vector<vector<int>> G(k, vector<int>(n, 0));

    for (int i = 0; i < k; i++)
    {
        // Form x^(r+i)
        vector<int> x_power(r + i + 1, 0);
        x_power[r + i] = 1;

        // Compute remainder of x^(r+i) / g(x)
        vector<int> rem = PolyModGF2(x_power, gen_poly);

        // Row i of G: the parity bits (remainder) in positions [0..r-1],
        // then identity bit at position r+i
        // Systematic form: [P | I_k] where data bits are in the last k positions
        for (int j = 0; j < (int)rem.size(); j++)
            G[i][j] = rem[j];
        G[i][r + i] = 1; // Identity part
    }

    return G;
}

/**
 * @brief Creates a generator matrix for a BCH code with distance 5.
 * g(x) = lcm(M1(x), M3(x)) over GF(2^m), n = 2^m - 1.
 */
static vector<vector<int>> BinaryGenMat5(const int n)
{
    assert(n >= 7); // Minimum meaningful length
    // Find smallest m such that 2^m - 1 >= n
    int m = 1;
    while ((1 << m) - 1 < n)
        m++;

    int n_bch = (1 << m) - 1;
    vector<int> prim_poly = GetPrimitivePoly(m);

    // Compute minimal polynomials M1 and M3
    vector<int> M1 = MinimalPoly(m, 1, prim_poly);
    vector<int> M3 = MinimalPoly(m, 3, prim_poly);

    // g(x) = lcm(M1, M3)
    vector<int> gen_poly = PolyLCM_GF2(M1, M3);

    // Build generator matrix
    vector<vector<int>> G = GenMatFromPoly(gen_poly, n_bch);

    // Shorten if needed
    if (n_bch > n)
        G = BinaryShorten(G, n_bch - n);

    return G;
}

/**
 * @brief Creates a generator matrix for an extended BCH code with distance 6.
 * Takes a d=5 BCH code of length n-1 and adds an overall parity bit.
 */
static vector<vector<int>> BinaryGenMat6(const int n)
{
    assert(n >= 8);
    // Build d=5 code of length n-1
    vector<vector<int>> G5 = BinaryGenMat5(n - 1);
    int k = G5.size();

    // Extend: append overall parity column
    vector<vector<int>> G(k, vector<int>(n, 0));
    for (int i = 0; i < k; i++)
    {
        int parity = 0;
        for (int j = 0; j < n - 1; j++)
        {
            G[i][j] = G5[i][j];
            parity ^= G5[i][j];
        }
        G[i][n - 1] = parity;
    }

    return G;
}

/**
 * @brief Creates a generator matrix for a BCH code with distance 7.
 * g(x) = lcm(M1(x), M3(x), M5(x)) over GF(2^m), n = 2^m - 1.
 */
static vector<vector<int>> BinaryGenMat7(const int n)
{
    assert(n >= 10); // Minimum meaningful length
    int m = 1;
    while ((1 << m) - 1 < n)
        m++;

    int n_bch = (1 << m) - 1;
    vector<int> prim_poly = GetPrimitivePoly(m);

    vector<int> M1 = MinimalPoly(m, 1, prim_poly);
    vector<int> M3 = MinimalPoly(m, 3, prim_poly);
    vector<int> M5 = MinimalPoly(m, 5, prim_poly);

    vector<int> gen_poly = PolyLCM_GF2(PolyLCM_GF2(M1, M3), M5);

    vector<vector<int>> G = GenMatFromPoly(gen_poly, n_bch);

    if (n_bch > n)
        G = BinaryShorten(G, n_bch - n);

    return G;
}

// ============================================================================
// Encoding
// ============================================================================

/**
 * @brief Generates all possible binary data vectors of a given length.
 * @param dataLen The length of the vectors to generate.
 * @return A vector containing all 2^dataLen possible binary data vectors.
 */
static vector<vector<int>> BinaryDataVecs(const int dataLen)
{
    vector<vector<int>> result;
    if (dataLen <= 0)
        return result;

    vector<int> start(dataLen, 0);
    for (vector<int> vec = start; !vec.empty(); vec = NextBase2(vec))
    {
        result.push_back(vec);
    }
    return result;
}

/**
 * @brief Encodes binary data vectors into codewords using the generator matrix.
 */
static void BinaryCodeVecs(const vector<vector<int>> &rawVecs, vector<vector<int>> &codedVecs,
                           const int n, const int minHammDist,
                           const vector<int> &bias, const vector<int> &row_perm, const vector<int> &col_perm)
{
    assert((minHammDist >= 2) && (minHammDist <= 7));
    vector<vector<int>> genMat;
    int k = 0;

    switch (minHammDist)
    {
    case 2:
        genMat = BinaryGenMat2(n);
        k = n - 1;
        break;
    case 3:
        genMat = BinaryGenMat3(n);
        k = genMat.size();
        break;
    case 4:
        genMat = BinaryGenMat4(n);
        k = genMat.size();
        break;
    case 5:
        genMat = BinaryGenMat5(n);
        k = genMat.size();
        break;
    case 6:
        genMat = BinaryGenMat6(n);
        k = genMat.size();
        break;
    case 7:
        genMat = BinaryGenMat7(n);
        k = genMat.size();
        break;
    default:
        assert(0);
    }

    // Apply permutations
    genMat = BinaryPermuteColumns(genMat, col_perm);
    genMat = BinaryPermuteRows(genMat, row_perm);

    // Determine if bias should be applied
    bool useBias = !bias.empty();
    if (useBias)
    {
        std::cout << "Using binary bias: ";
        for (const int &b : bias)
            std::cout << b << " ";
        std::cout << std::endl;
    }
    else
    {
        std::cout << "Not using bias addition." << std::endl;
    }
    std::cout << std::endl;

    // Encode each raw data vector
    for (const vector<int> &rawVec : rawVecs)
    {
        assert((int)rawVec.size() == k);
        vector<int> codedVec = MatMulGF2(rawVec, genMat, k, n);

        // Apply bias via XOR
        if (useBias)
        {
            for (int i = 0; i < n; i++)
            {
                codedVec[i] ^= bias[i];
            }
        }

        codedVecs.push_back(codedVec);
    }
}

// ============================================================================
// Public API
// ============================================================================

vector<vector<int>> BinaryCodedVecs(const int n, const int minHammDist, const vector<int> &bias,
                                     const vector<int> &row_perm, const vector<int> &col_perm)
{
    assert((minHammDist >= 2) && (minHammDist <= 7));

    // Determine the dimension k by building the generator matrix
    // (We let BinaryCodeVecs handle matrix construction internally)
    // First compute k to know how many data vectors to enumerate
    int k = calculateBinaryRowPermSize(n, minHammDist);
    assert(k > 0);

    // Generate all possible binary data vectors of length k
    vector<vector<int>> rawVecs = BinaryDataVecs(k);

    // Encode
    vector<vector<int>> codedVecs;
    codedVecs.reserve(rawVecs.size());
    BinaryCodeVecs(rawVecs, codedVecs, n, minHammDist, bias, row_perm, col_perm);
    return codedVecs;
}

vector<vector<int>> BinaryCodedVecs(const int n, const int minHammDist)
{
    vector<int> bias(n, 0);
    vector<int> row_perm, col_perm;

    int k = calculateBinaryRowPermSize(n, minHammDist);

    row_perm.resize(k);
    col_perm.resize(n);
    for (int i = 0; i < k; ++i)
        row_perm[i] = i;
    for (int i = 0; i < n; ++i)
        col_perm[i] = i;

    return BinaryCodedVecs(n, minHammDist, bias, row_perm, col_perm);
}
