#include "Candidates/DifferentialVTCodes.hpp"
#include "Utils.hpp" // Assumes VecToStr and NextBase4 are here
#include <climits>   // For ULLONG_MAX
#include <numeric>
#include <thread>
#include <vector>

namespace
{

// The base q is fixed at 4 for 4-ary words.
constexpr int Q_BASE = 4;

/**
 * @brief Converts a large integer index into a base-4 word of length n.
 * (Copied from VTCodes.cpp)
 */
std::vector<int> indexToWord(unsigned long long index, int n)
{
    if (n <= 0)
        return {};
    std::vector<int> word(n);
    unsigned long long temp_index = index;
    for (int i = n - 1; i >= 0; --i)
    {
        word[i] = temp_index % Q_BASE;
        temp_index /= Q_BASE;
    }
    return word;
}

/**
 * @brief Checks if a single word satisfies the D-VT syndrome condition.
 */
bool checkDifferentialWord(const std::vector<int> &word, int n, int s)
{
    if (n <= 0)
        return (s == 0); // Empty word has syndrome 0

    // Modulus is n*q
    long long modulus = (long long)n * Q_BASE;
    if (modulus <= 0)
        return false; // Should not happen if n > 0

    // 1. Compute the differential vector y
    std::vector<int> diff_vec(n);
    for (int i = 0; i < n - 1; ++i)
    {
        int diff = word[i] - word[i + 1];
        // Ensure correct modulo for negative numbers
        diff_vec[i] = ((diff % Q_BASE) + Q_BASE) % Q_BASE;
    }
    diff_vec[n - 1] = word[n - 1]; // y_n = x_n

    // 2. Calculate the syndrome: Sum_{i=1 to n} (i * y_i)
    long long syndrome_sum = 0;
    for (int i = 0; i < n; ++i)
    {
        // Use (i+1) for 1-based indexing
        syndrome_sum += (long long)(i + 1) * diff_vec[i];
    }

    // 3. Check if syndrome matches s (mod nq)
    // Use robust modulo comparison
    long long target_s = ((s % modulus) + modulus) % modulus;
    long long calculated_s = ((syndrome_sum % modulus) + modulus) % modulus;

    return calculated_s == target_s;
}

/**
 * @brief The worker function executed by each thread.
 */
void findCodeWordsWorker(int n, int s, unsigned long long start_index, unsigned long long count,
                         std::vector<std::string> *results)
{
    if (count == 0)
        return;

    std::vector<int> current_word = indexToWord(start_index, n);

    for (unsigned long long i = 0; i < count; ++i)
    {
        if (checkDifferentialWord(current_word, n, s))
        {
            results->push_back(VecToStr(current_word));
        }

        if (i < count - 1)
        {
            current_word = NextBase4(current_word);
        }
    }
}

} // end anonymous namespace

std::vector<std::string> GenerateDifferentialVTCodes(int n, int s, unsigned int num_threads)
{
    if (n <= 0)
    {
        return checkDifferentialWord({}, n, s) ? std::vector<std::string>{""} : std::vector<std::string>{};
    }

    unsigned int threads_to_use = num_threads;
    if (threads_to_use == 0)
    {
        threads_to_use = std::thread::hardware_concurrency();
        if (threads_to_use == 0)
            threads_to_use = 2; // Default
    }

    unsigned long long total_words = 1;
    for (int i = 0; i < n; ++i)
    {
        if (ULLONG_MAX / Q_BASE < total_words)
        {
            return {}; // Overflow, too large to process.
        }
        total_words *= Q_BASE;
    }

    if (total_words < 10000)
    {
        threads_to_use = 1;
    }

    std::vector<std::thread> threads;
    threads.reserve(threads_to_use);
    std::vector<std::vector<std::string>> thread_results(threads_to_use);

    unsigned long long start_index = 0;
    unsigned long long words_per_thread = total_words / threads_to_use;

    for (unsigned int i = 0; i < threads_to_use; ++i)
    {
        unsigned long long count = (i == threads_to_use - 1) ? (total_words - start_index) : words_per_thread;

        threads.emplace_back(findCodeWordsWorker, n, s, start_index, count, &thread_results[i]);
        start_index += count;
    }

    for (auto &t : threads)
    {
        t.join();
    }

    std::vector<std::string> final_result;
    size_t total_size = 0;
    for (const auto &res : thread_results)
    {
        total_size += res.size();
    }
    final_result.reserve(total_size);

    for (auto &res : thread_results)
    {
        final_result.insert(final_result.end(), std::make_move_iterator(res.begin()),
                            std::make_move_iterator(res.end()));
    }

    return final_result;
}