#include "Candidates/VTCodes.hpp"
#include "Utils.hpp"
#include <climits> // For ULLONG_MAX
#include <numeric>
#include <thread>
#include <vector>

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
 * @brief Checks if a single word satisfies the two required conditions.
 */
bool checkWord(const std::vector<int> &word, int n, int a, int b)
{
    // Condition 1: sum_{i=2 to n} (i-1)*alpha_i ≡ a (mod n)
    long long alpha_sum = 0;
    if (n >= 2)
    {
        for (int i = 2; i <= n; ++i)
        {
            if (word[i - 1] >= word[i - 2])
            {
                alpha_sum += (i - 1);
            }
        }
    }
    if (n > 0 && ((alpha_sum % n) + n) % n != ((a % n) + n) % n)
    {
        return false;
    }
    if (n == 0 && a != 0)
        return false; // Edge case for n=0

    // Condition 2: sum_{j=1 to n} x_j ≡ b (mod 4)
    long long element_sum = 0;
    for (int digit : word)
    {
        element_sum += digit;
    }
    if (((element_sum % Q_BASE) + Q_BASE) % Q_BASE != ((b % Q_BASE) + Q_BASE) % Q_BASE)
    {
        return false;
    }

    return true;
}

/**
 * @brief The worker function executed by each thread.
 */
void findCodeWordsWorker(int n, int a, int b, unsigned long long start_index, unsigned long long count,
                         std::vector<std::string> *results)
{
    if (count == 0)
        return;

    std::vector<int> current_word = indexToWord(start_index, n);

    for (unsigned long long i = 0; i < count; ++i)
    {
        if (checkWord(current_word, n, a, b))
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

std::vector<std::string> GenerateVTCodes(int n, int a, int b, unsigned int num_threads)
{
    if (n <= 0)
    {
        // A single empty word might satisfy the condition if n=0, a=0, b=0.
        return checkWord({}, n, a, b) ? std::vector<std::string>{""} : std::vector<std::string>{};
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

        threads.emplace_back(findCodeWordsWorker, n, a, b, start_index, count, &thread_results[i]);
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
