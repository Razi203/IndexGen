/**
 * @file WaveGen.cpp
 * @brief Implements the Progressive Wave Construction method.
 */

#include "Candidates/WaveGen.hpp"
#include "IndexGen.hpp"
#include "Utils.hpp" // For FastEditDistance
#include <algorithm>
#include <cmath> // For exp()
#include <iostream>
#include <numeric> // For std::iota
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

// --- Private Helper Prototypes ---
string GenerateRandomString(int len, mt19937 &rng);
int HammingDistance(const string &s1, const string &s2);
string RepairCandidate(const string &bad_candidate, const string &conflict, mt19937 &rng);
int CalculateTotalConflicts(const vector<string> &pool, int min_hd);

/**
 * @brief Implements "Simulated Annealing with Conflict-Targeted Repair".
 * This is the most advanced method, designed to escape local optima.
 * This version is optimized to be significantly faster.
 */
vector<string> GenProgressiveWave(const Params &params)
{
    auto *constraints = dynamic_cast<ProgressiveWaveConstraints *>(params.constraints.get());
    if (!constraints)
    {
        throw runtime_error("Invalid constraints for PROGRESSIVE_WAVE method.");
    }

    int code_len = params.codeLen;
    int target_pool_size = constraints->pool_size;
    int min_hd = constraints->num_seeds;

    // --- Simulated Annealing Parameters ---
    double start_temp = 5.0;
    double end_temp = 0.01;
    double cooling_rate = 0.995;
    // More steps are feasible now due to the performance optimization
    int steps_per_temp = target_pool_size * 5;

    cout << "Starting Optimized Simulated Annealing..." << endl;
    cout << "  - Target Pool Size: " << target_pool_size << endl;
    cout << "  - Minimum Hamming Distance: " << min_hd << endl;

    mt19937 rng(random_device{}());

    // --- Step 1: Generate the initial random pool ---
    unordered_set<string> initial_set;
    while (initial_set.size() < static_cast<size_t>(target_pool_size))
    {
        initial_set.insert(GenerateRandomString(code_len, rng));
    }
    vector<string> current_pool(initial_set.begin(), initial_set.end());
    cout << "  - Generated initial pool of " << current_pool.size() << " unique candidates." << endl;

    // --- Step 2: Main Annealing Loop (Optimized) ---
    double temp = start_temp;
    uniform_real_distribution<double> accept_dist(0.0, 1.0);
    uniform_int_distribution<int> candidate_dist(0, current_pool.size() - 1);

    int initial_conflicts = CalculateTotalConflicts(current_pool, min_hd);
    cout << "  - Initial Conflicts: " << initial_conflicts << endl;

    while (temp > end_temp)
    {
        for (int i = 0; i < steps_per_temp; ++i)
        {
            // a) Pick a random candidate
            int candidate_idx = candidate_dist(rng);
            const string &candidate_to_check = current_pool[candidate_idx];

            // b) Find its specific conflicts
            vector<string> current_conflicts;
            for (int j = 0; j < static_cast<int>(current_pool.size()); ++j)
            {
                if (candidate_idx == j)
                    continue;
                if (HammingDistance(candidate_to_check, current_pool[j]) < min_hd)
                {
                    current_conflicts.push_back(current_pool[j]);
                }
            }

            if (current_conflicts.empty())
            {
                continue; // This candidate is fine, try another one
            }

            // c) Generate a repaired version
            string repaired_candidate = RepairCandidate(candidate_to_check, current_conflicts[0], rng);

            // d) Calculate the change in conflicts (delta E)
            int new_conflicts = 0;
            for (int j = 0; j < static_cast<int>(current_pool.size()); ++j)
            {
                if (candidate_idx == j)
                    continue;
                if (HammingDistance(repaired_candidate, current_pool[j]) < min_hd)
                {
                    new_conflicts++;
                }
            }

            int delta_conflicts = new_conflicts - current_conflicts.size();

            // e) Metropolis acceptance criterion
            if (delta_conflicts < 0 || accept_dist(rng) < exp(-delta_conflicts / temp))
            {
                current_pool[candidate_idx] = repaired_candidate;
            }
        }
        cout << "  - Temp: " << temp << ", Conflicts: " << CalculateTotalConflicts(current_pool, min_hd) << endl;
        temp *= cooling_rate; // Cool down
    }

    // --- Step 3: Final "Freeze" - A purely greedy pass to guarantee validity ---
    cout << "--- Annealing Complete. Performing final greedy freeze... ---" << endl;
    vector<string> final_pool;
    final_pool.reserve(current_pool.size());
    shuffle(current_pool.begin(), current_pool.end(), rng);

    for (const auto &candidate : current_pool)
    {
        bool is_valid = true;
        for (const auto &final_member : final_pool)
        {
            if (HammingDistance(candidate, final_member) < min_hd)
            {
                is_valid = false;
                break;
            }
        }
        if (is_valid)
        {
            final_pool.push_back(candidate);
        }
    }

    cout << "  - Final candidate set size: " << final_pool.size() << endl;
    return final_pool;
}

/**
 * @brief Repairs a candidate by making a targeted change to resolve a conflict.
 * To increase Hamming distance, it must change a character at a position
 * where the two strings are currently the SAME.
 */
string RepairCandidate(const string &bad_candidate, const string &conflict, mt19937 &rng)
{
    string repaired = bad_candidate;
    vector<int> same_indices;
    for (size_t i = 0; i < bad_candidate.length(); ++i)
    {
        if (bad_candidate[i] == conflict[i])
        {
            same_indices.push_back(i);
        }
    }

    if (!same_indices.empty())
    {
        uniform_int_distribution<int> dist(0, same_indices.size() - 1);
        int pos_to_change = same_indices[dist(rng)];
        char current_char = repaired[pos_to_change];
        char new_char;
        uniform_int_distribution<int> char_dist(0, 3);
        do
        {
            new_char = char_dist(rng) + '0';
        } while (new_char == current_char);
        repaired[pos_to_change] = new_char;
    }
    // If there are no same indices (max possible distance), just make a random change.
    else
    {
        uniform_int_distribution<int> pos_dist(0, repaired.length() - 1);
        int pos_to_change = pos_dist(rng);
        char current_char = repaired[pos_to_change];
        char new_char;
        uniform_int_distribution<int> char_dist(0, 3);
        do
        {
            new_char = char_dist(rng) + '0';
        } while (new_char == current_char);
        repaired[pos_to_change] = new_char;
    }
    return repaired;
}

/**
 * @brief Calculates the total number of conflicting pairs in a pool.
 */
int CalculateTotalConflicts(const vector<string> &pool, int min_hd)
{
    int conflict_count = 0;
    for (size_t i = 0; i < pool.size(); ++i)
    {
        for (size_t j = i + 1; j < pool.size(); ++j)
        {
            if (HammingDistance(pool[i], pool[j]) < min_hd)
            {
                conflict_count++;
            }
        }
    }
    return conflict_count;
}

/**
 * @brief Calculates the Hamming distance between two strings.
 * @note Assumes strings are of equal length for performance. No safety checks.
 */
int HammingDistance(const string &s1, const string &s2)
{
    int dist = 0;
    for (size_t i = 0; i < s1.length(); ++i)
    {
        if (s1[i] != s2[i])
        {
            dist++;
        }
    }
    return dist;
}

/**
 * @brief Generates a random string of a given length using the alphabet {0, 1, 2, 3}.
 */
string GenerateRandomString(int len, mt19937 &rng)
{
    const char alphabet[] = {'0', '1', '2', '3'};
    uniform_int_distribution<int> dist(0, 3);
    string result = "";
    result.reserve(len);
    for (int i = 0; i < len; ++i)
    {
        result += alphabet[dist(rng)];
    }
    return result;
}
