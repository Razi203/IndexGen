#include "Custom1.hpp"
#include "Utils.hpp"
#include <thread>
#include <vector>
#include <numeric>
#include <climits> // For ULLONG_MAX

namespace
{
    // The base q is fixed at 4 for 4-ary words.
    constexpr int Q_BASE = 4;

    /**
     * @brief Converts a large integer index into a base-4 word of length n.
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
     * @brief Checks if a single word satisfies the required condition.
     * The condition is: sum_{i=1 to n} (i * x_i) ≡ a (mod n + 1)
     */
    bool checkWord(const std::vector<int> &word, int n, int a)
    {
        long long weighted_sum = 0;
        // The sum is from i=1 to n. The word vector is 0-indexed.
        // So, x_i corresponds to word[i-1].
        for (int i = 1; i <= n; ++i)
        {
            weighted_sum += (long long)i * word[i - 1];
        }

        int modulus = n + 1;
        if (modulus <= 0)
            return false;

        // Check congruency, handling negative results from the % operator in C++.
        return ((weighted_sum % modulus) + modulus) % modulus == ((a % modulus) + modulus) % modulus;
    }

    /**
     * @brief The worker function executed by each thread. It iterates through a
     * range of possible words and checks if they are valid codewords.
     */
    void findCodeWordsWorker(
        int n, int a,
        unsigned long long start_index,
        unsigned long long count,
        std::vector<std::string> *results)
    {
        if (count == 0)
            return;

        std::vector<int> current_word = indexToWord(start_index, n);

        for (unsigned long long i = 0; i < count; ++i)
        {
            if (checkWord(current_word, n, a))
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

std::vector<std::string> GenerateCustomCodes(int n, int a, unsigned int num_threads)
{
    if (n < 0)
    {
        return {};
    }
    if (n == 0)
    {
        return {""};
    }

    unsigned int threads_to_use = num_threads;
    if (threads_to_use == 0)
    {
        threads_to_use = std::thread::hardware_concurrency();
        if (threads_to_use == 0)
            threads_to_use = 2;
    }

    unsigned long long total_words = 1;
    for (int i = 0; i < n; ++i)
    {
        if (ULLONG_MAX / Q_BASE < total_words)
        {
            return {};
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
        unsigned long long count = (i == threads_to_use - 1)
                                       ? (total_words - start_index)
                                       : words_per_thread;

        threads.emplace_back(
            findCodeWordsWorker, n, a, start_index, count, &thread_results[i]);
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
        final_result.insert(
            final_result.end(),
            std::make_move_iterator(res.begin()),
            std::make_move_iterator(res.end()));
    }

    return final_result;
}
