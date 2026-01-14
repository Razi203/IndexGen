/**
 * @file SparseMat.hpp
 * @brief Defines the data structures and algorithms for generating a codebook from candidate strings using a sparse
 * matrix (adjacency list) representation of a conflict graph.
 *
 * This file's primary purpose is to implement a workflow for DNA sequence codebook generation. The process involves:
 * 1.  Constructing a "conflict graph" where an edge exists between any two candidate strings if their edit distance is
 * below a specified threshold. This graph is represented by the `AdjList` class.
 * 2.  Applying a greedy algorithm to find a maximal independent set of vertices in this graph. This set of vertices
 * corresponds to a "codebook" of strings where every pair satisfies the minimum edit distance constraint.
 * 3.  Providing functionality to save and resume this potentially long-running computation.
 */

#ifndef SPARSEMAT_HPP_
#define SPARSEMAT_HPP_

#include "IndexGen.hpp" // Assumed to contain the definition for Params struct
#include <chrono>       // Use modern C++ time library
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Forward declaration for the Params struct defined in IndexGen.hpp
struct Params;

// =================================================================================
// SECTION: AdjList CLASS - Sparse Matrix Representation
// =================================================================================

/**
 * @class AdjList
 * @brief Implements a sparse symmetric matrix using an adjacency list, optimized for finding and removing nodes with
 * the minimum degree.
 *
 * This class is the core data structure for representing the conflict graph.
 * An edge (i, j) exists if the strings corresponding to indices i and j are too similar.
 * The main algorithm iteratively selects the node with the fewest conflicts (minimum degree),
 * adds it to the codebook, and removes it and its neighbors from the graph.
 */
class AdjList
{
  private:
    /**
     * @brief The primary data structure for the adjacency list.
     * `m[i]` is an unordered_set containing the indices of all nodes adjacent to node `i`.
     */
    std::unordered_map<int, std::unordered_set<int>> m;

    /**
     * @brief An optimization structure to quickly find nodes with the minimum degree.
     * The map's key is the degree (row sum), and the value is a set of all row indices
     * that currently have that degree. This avoids iterating through the entire graph
     * to find the minimum-degree node.
     */
    std::map<int, std::unordered_set<int>> rowsBySum;

  public:
    // --- Public Member Functions ---

    /**
     * @brief Populates the `rowsBySum` optimization map based on the current state of the adjacency list `m`.
     * This should be called once after the graph is fully constructed.
     */
    void RowsBySum();

    /**
     * @brief Finds and returns the index of a node with the current minimum degree.
     * @return The integer index of a row/node with the minimum sum. Asserts if the graph is empty.
     */
    int MinSumRow() const;

    /**
     * @brief Finds and returns the index of a node with the current maximum degree.
     * @return The integer index of a row/node with the maximum sum. Asserts if the graph is empty.
     */
    int MaxSumRow() const;

    /**
     * @brief Updates `rowsBySum` by moving a row from its current degree-group to the next lower one.
     * @param currentSum The current degree of the row.
     * @param row The index of the row whose degree has decreased.
     */
    void DecreaseSum(const int currentSum, const int row);

    /**
     * @brief Removes a row entirely from the `rowsBySum` map.
     * @param currentSum The degree of the row being removed.
     * @param row The index of the row to remove.
     */
    void DeleteRow(const int currentSum, const int row);

    /**
     * @brief Removes all rows with zero degree from the graph.
     * @return number of removed rows
     */
    int RemoveEmptyRows();

    /**
     * @brief Checks if the graph is empty.
     * @return `true` if the adjacency list `m` is empty, `false` otherwise.
     */
    bool empty() const;

    /**
     * @brief Gets the number of nodes (rows) currently in the graph.
     * @return The number of rows in the adjacency list `m`.
     */
    int RowNum() const;

    /**
     * @brief Adds an edge to the graph by setting the matrix entry (row, col) to 1.
     * Since the graph is symmetric, you typically call Set(row, col) and Set(col, row).
     * @param row The index of the first node.
     * @param col The index of the second node.
     */
    void Set(int row, int col);

    /**
     * @brief Removes a node and all its incident edges from the graph.
     * @details This is a complex operation that involves:
     * 1. Finding all neighbors `j` of the node `rc`.
     * 2. For each neighbor `j`, removing the edge back to `rc`.
     * 3. Updating the degree of each neighbor `j` in `rowsBySum`.
     * 4. Finally, removing the node `rc` itself from `m` and `rowsBySum`.
     * @param rc The index of the row/column (node) to delete.
     */
    void DelRowCol(int rc);

    /**
     * @brief Removes a node and its entire 1-hop neighborhood (a "ball" around it).
     * @details This is the core operation of the greedy algorithm. When a node `matRow` is
     * chosen for the codebook, this function removes `matRow` itself and all of its
     * neighbors, as none of them can be chosen anymore.
     * @param matRow The index of the central node of the ball to delete.
     * @param remaining A set of all remaining candidate indices, which is updated by this function.
     */
    void DelBall(const int matRow, std::unordered_set<int> &remaining);

    /**
     * @brief Finds the minimum-degree node and then deletes its corresponding ball.
     * @param remaining A set of remaining candidate indices, updated by `DelBall`.
     * @param minSumRowTime A double to accumulate the total seconds spent in `MinSumRow`.
     * @param delBallTime A double to accumulate the total seconds spent in `DelBall`.
     * @return The index of the minimum-degree node that was chosen and removed.
     */
    int FindMinDel(std::unordered_set<int> &remaining, double &minSumRowTime, double &delBallTime);

    /**
     * @brief Finds the maximum-degree node and removes only that node (not its neighbors).
     * @param remaining A set of remaining candidate indices, updated by removing the max node.
     * @param maxSumRowTime A double to accumulate the total seconds spent in finding the max sum row.
     * @param delRowColTime A double to accumulate the total seconds spent in `DelRowCol`.
     * @return The index of the maximum-degree node that was chosen and removed.
     */
    int FindMaxDel(std::unordered_set<int> &remaining, double &maxSumRowTime, double &delRowColTime);

    /**
     * @brief Serializes the adjacency list to a file.
     * @param filename The name of the file to save to.
     */
    void ToFile(const std::string &filename) const;

    /**
     * @brief Deserializes the adjacency list from a file.
     * @param filename The name of the file to load from.
     */
    void FromFile(const std::string &filename);

    /**
     * @brief Deserializes the adjacency list from a BINARY file (generated by Python GPU script).
     * @param filename The name of the file to load from.
     */
    void FromBinaryFile(const std::string &filename, long long int &matrixOnesNum, bool silent = false);
};

// =================================================================================
// SECTION: CODEBOOK GENERATION WORKFLOW
// =================================================================================

/**
 * @brief Main function to generate a codebook from a set of initial parameters.
 * @details This function orchestrates the entire process:
 * 1. Generates a set of candidate strings based on `params`.
 * 2. Builds the conflict graph (`AdjList`) where edges connect candidates with an edit distance < `minED`.
 * 3. Runs a greedy algorithm on the graph to extract a large codebook (maximal independent set).
 * 4. Saves the final codebook and performance statistics to a file.
 * This process is resumable.
 * @param params A struct containing all generation parameters (e.g., string length, min distance).
 */
void GenerateCodebookAdj(const Params &params);

/**
 * @brief Resumes a previously interrupted codebook generation process from saved progress files.
 * @details It reads files like "progress_params.txt", "progress_stage.txt", etc.,
 * to restore the state and continue the computation, preventing loss of work.
 */
void GenerateCodebookAdjResumeFromFile();

#endif /* SPARSEMAT_HPP_ */