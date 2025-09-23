#include "VTCodes.hpp"
#include <thread>
#include <mutex>
#include <functional>
#include <numeric>

//-----------------------------------------------------------------------------
// Single-Threaded Implementation (In-Memory)
//-----------------------------------------------------------------------------

namespace
{
    /**
     * @brief Internal recursive helper for the single-threaded generator.
     */
    void generate_recursive_st(int n, int a, std::vector<std::string> &result, std::string &current_word, long long current_sum)
    {
        int depth = current_word.length();
        if (depth == n)
        {
            long long modulus = n + 1;
            if ((current_sum % modulus + modulus) % modulus == a)
            {
                result.push_back(current_word);
            }
            return;
        }

        for (int i = 0; i < 4; ++i)
        {
            current_word.push_back('0' + i);
            generate_recursive_st(n, a, result, current_word, current_sum + (long long)(depth + 1) * i);
            current_word.pop_back(); // Backtrack
        }
    }
} // namespace

std::vector<std::string> GenerateVTCodes(int n, int a)
{
    std::vector<std::string> result;
    if (n <= 0)
    {
        return result;
    }
    std::string current_word = "";
    current_word.reserve(n);
    generate_recursive_st(n, a, result, current_word, 0);
    return result;
}

//-----------------------------------------------------------------------------
// Multi-Threaded Implementation (Stream-based for Files/Console)
//-----------------------------------------------------------------------------

namespace
{
    /**
     * @brief Internal recursive worker for the multi-threaded stream generator.
     */
    void generate_recursive_mt_stream(int n, int a, std::ostream &out, std::mutex &out_mutex, std::string &current_word, long long current_sum)
    {
        int depth = current_word.length();
        if (depth == n)
        {
            long long modulus = n + 1;
            if ((current_sum % modulus + modulus) % modulus == a)
            {
                std::lock_guard<std::mutex> lock(out_mutex);
                out << current_word << "\n";
            }
            return;
        }

        for (int i = 0; i < 4; ++i)
        {
            current_word.push_back('0' + i);
            generate_recursive_mt_stream(n, a, out, out_mutex, current_word, current_sum + (long long)(depth + 1) * i);
            current_word.pop_back();
        }
    }

    /**
     * @brief Worker function for each thread (stream version).
     */
    void thread_worker_stream(int n, int a, std::ostream &out, std::mutex &out_mutex, const std::vector<std::string> &prefixes_for_thread)
    {
        for (const auto &prefix : prefixes_for_thread)
        {
            std::string current_word = prefix;
            current_word.reserve(n);
            long long current_sum = 0;
            for (size_t i = 0; i < prefix.length(); ++i)
            {
                current_sum += (long long)(i + 1) * (prefix[i] - '0');
            }
            generate_recursive_mt_stream(n, a, out, out_mutex, current_word, current_sum);
        }
    }
} // namespace

void GenerateVTCodesMT(int n, int a, std::ostream &out, unsigned int num_threads)
{
    if (n <= 0)
        return;
    if (num_threads == 0)
        num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0)
        num_threads = 1;

    std::vector<std::thread> threads;
    std::mutex out_mutex;

    int prefix_len = 1;
    unsigned long long num_prefixes = 4;
    while (num_prefixes < num_threads && prefix_len < n)
    {
        num_prefixes *= 4;
        prefix_len++;
    }

    std::vector<std::string> all_prefixes;
    std::function<void(std::string)> generate_prefixes =
        [&](std::string current_prefix)
    {
        if ((int)current_prefix.length() == prefix_len)
        {
            all_prefixes.push_back(current_prefix);
            return;
        }
        for (int i = 0; i < 4; ++i)
        {
            generate_prefixes(current_prefix + std::to_string(i));
        }
    };
    generate_prefixes("");

    std::vector<std::vector<std::string>> thread_workloads(num_threads);
    for (size_t i = 0; i < all_prefixes.size(); ++i)
    {
        thread_workloads[i % num_threads].push_back(all_prefixes[i]);
    }

    for (unsigned int i = 0; i < num_threads; ++i)
    {
        if (!thread_workloads[i].empty())
        {
            threads.emplace_back(thread_worker_stream, n, a, std::ref(out), std::ref(out_mutex), std::cref(thread_workloads[i]));
        }
    }

    for (auto &t : threads)
    {
        if (t.joinable())
            t.join();
    }
}

//-----------------------------------------------------------------------------
// Multi-Threaded Implementation (In-Memory)
//-----------------------------------------------------------------------------

namespace
{
    /**
     * @brief Internal recursive worker for the multi-threaded memory generator.
     */
    void generate_recursive_mt_mem(int n, int a, std::vector<std::string> &result, std::mutex &result_mutex, std::string &current_word, long long current_sum)
    {
        int depth = current_word.length();
        if (depth == n)
        {
            long long modulus = n + 1;
            if ((current_sum % modulus + modulus) % modulus == a)
            {
                std::lock_guard<std::mutex> lock(result_mutex);
                result.push_back(current_word);
            }
            return;
        }

        for (int i = 0; i < 4; ++i)
        {
            current_word.push_back('0' + i);
            generate_recursive_mt_mem(n, a, result, result_mutex, current_word, current_sum + (long long)(depth + 1) * i);
            current_word.pop_back();
        }
    }

    /**
     * @brief Worker function for each thread (memory version).
     */
    void thread_worker_mem(int n, int a, std::vector<std::string> &result, std::mutex &result_mutex, const std::vector<std::string> &prefixes_for_thread)
    {
        for (const auto &prefix : prefixes_for_thread)
        {
            std::string current_word = prefix;
            current_word.reserve(n);
            long long current_sum = 0;
            for (size_t i = 0; i < prefix.length(); ++i)
            {
                current_sum += (long long)(i + 1) * (prefix[i] - '0');
            }
            generate_recursive_mt_mem(n, a, result, result_mutex, current_word, current_sum);
        }
    }
} // namespace

std::vector<std::string> GenerateVTCodesMT_Mem(int n, int a, unsigned int num_threads)
{
    std::vector<std::string> result;
    if (n <= 0)
        return result;
    if (num_threads == 0)
        num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0)
        num_threads = 1;

    std::vector<std::thread> threads;
    std::mutex result_mutex;

    int prefix_len = 1;
    unsigned long long num_prefixes = 4;
    while (num_prefixes < num_threads && prefix_len < n)
    {
        num_prefixes *= 4;
        prefix_len++;
    }

    std::vector<std::string> all_prefixes;
    std::function<void(std::string)> generate_prefixes =
        [&](std::string current_prefix)
    {
        if ((int)current_prefix.length() == prefix_len)
        {
            all_prefixes.push_back(current_prefix);
            return;
        }
        for (int i = 0; i < 4; ++i)
        {
            generate_prefixes(current_prefix + std::to_string(i));
        }
    };
    generate_prefixes("");

    std::vector<std::vector<std::string>> thread_workloads(num_threads);
    for (size_t i = 0; i < all_prefixes.size(); ++i)
    {
        thread_workloads[i % num_threads].push_back(all_prefixes[i]);
    }

    for (unsigned int i = 0; i < num_threads; ++i)
    {
        if (!thread_workloads[i].empty())
        {
            threads.emplace_back(thread_worker_mem, n, a, std::ref(result), std::ref(result_mutex), std::cref(thread_workloads[i]));
        }
    }

    for (auto &t : threads)
    {
        if (t.joinable())
            t.join();
    }

    return result;
}
