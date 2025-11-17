/**
 * @file IndexGen.cpp
 * @brief The main entry point for the DNA codebook generator application.
 *
 * This file contains the `main` function that drives the entire process. It parses
 * command-line arguments, sets up the configuration parameters, creates a
 * working directory, initiates the codebook generation, and measures
 * the total execution time.
 */

#include <chrono>
#include <filesystem> // For directory creation
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

// This is a header-only library for command-line argument parsing.
#include "cxxopts.hpp"

#include "CandidateGenerator.hpp"
#include "Candidates.hpp"
#include "Decode.hpp"
#include "IndexGen.hpp"
#include "MaxClique.hpp"
#include "SparseMat.hpp"
#include "Utils.hpp"

using namespace std;

// --- Function Prototypes ---
void configure_parser(cxxopts::Options &options);
string get_timestamp();

/**
 * @brief The main function and entry point of the program.
 * hereinafter 0=A, 1=C, 2=G, 3=T
 *
 */
int main(int argc, char *argv[])
{
    cxxopts::Options options("IndexGen", "A flexible DNA codebook generator");
    try
    {
        configure_parser(options);
        auto result = options.parse(argc, argv);

        if (result.count("help"))
        {
            cout << options.help() << endl;
            return 0;
        }

        auto start_time = chrono::steady_clock::now();

        if (result.count("resume"))
        {
            // --- Resume Execution ---
            string dir_name = result["dir"].as<string>();
            if (dir_name.empty())
            {
                cerr << "Error: When resuming, the directory with save files must be specified using --dir." << endl;
                return 1;
            }

            filesystem::path work_dir(dir_name);
            if (!filesystem::exists(work_dir) || !filesystem::is_directory(work_dir))
            {
                cerr << "Error: Directory '" << dir_name << "' not found." << endl;
                return 1;
            }

            filesystem::current_path(work_dir);
            cout << "Resuming generation in directory: " << filesystem::current_path().string() << endl;
            GenerateCodebookAdjResumeFromFile();
        }
        else
        {
            // --- New Generation ---
            // --- Directory Setup ---
            string dir_name = result["dir"].as<string>();
            if (dir_name.empty())
            {
                dir_name = get_timestamp();
            }

            filesystem::path work_dir(dir_name);
            if (filesystem::exists(work_dir))
            {
                cerr << "Warning: Directory '" << dir_name << "' already exists. Files may be overwritten." << endl;
            }
            else
            {
                if (!filesystem::create_directory(work_dir))
                {
                    cerr << "Error: Could not create directory '" << dir_name << "'." << endl;
                    return 1;
                }
            }
            filesystem::current_path(work_dir);
            cout << "Output will be saved in directory: " << filesystem::current_path().string() << endl;

            // --- Parameter Configuration ---
            Params params;
            params.codeMinED = result["editDist"].as<int>();
            params.maxRun = result["maxRun"].as<int>();
            params.minGCCont = result["minGC"].as<double>();
            params.maxGCCont = result["maxGC"].as<double>();
            params.threadNum = result["threads"].as<int>();
            params.saveInterval = result["saveInterval"].as<int>();

            string method_str = result["method"].as<string>();

            if (method_str == "LinearCode")
            {
                params.method = GenerationMethod::LINEAR_CODE;
                int minHD = result["minHD"].as<int>();

                // Parse modes
                VectorMode bias_mode = parseVectorMode(result["lc_bias_mode"].as<string>(), "bias");
                VectorMode row_perm_mode = parseVectorMode(result["lc_row_perm_mode"].as<string>(), "row_perm");
                VectorMode col_perm_mode = parseVectorMode(result["lc_col_perm_mode"].as<string>(), "col_perm");

                // Parse manual vectors (if provided)
                vector<int> bias_vec, row_perm_vec, col_perm_vec;

                if (bias_mode == VectorMode::MANUAL)
                {
                    if (!result.count("lc_bias"))
                    {
                        throw runtime_error("--lc_bias required when lc_bias_mode=manual");
                    }
                    bias_vec = parseCSVVector(result["lc_bias"].as<string>(), "bias");
                }

                if (row_perm_mode == VectorMode::MANUAL)
                {
                    if (!result.count("lc_row_perm"))
                    {
                        throw runtime_error("--lc_row_perm required when lc_row_perm_mode=manual");
                    }
                    row_perm_vec = parseCSVVector(result["lc_row_perm"].as<string>(), "row_perm");
                }

                if (col_perm_mode == VectorMode::MANUAL)
                {
                    if (!result.count("lc_col_perm"))
                    {
                        throw runtime_error("--lc_col_perm required when lc_col_perm_mode=manual");
                    }
                    col_perm_vec = parseCSVVector(result["lc_col_perm"].as<string>(), "col_perm");
                }

                // Get random seed
                unsigned int random_seed = result["lc_random_seed"].as<unsigned int>();

                // Create constraints
                params.constraints = make_unique<LinearCodeConstraints>(minHD, bias_mode, row_perm_mode, col_perm_mode,
                                                                        bias_vec, row_perm_vec, col_perm_vec,
                                                                        random_seed);
            }
            else if (method_str == "VTCode")
            {
                params.method = GenerationMethod::VT_CODE;
                int rem_a = result["vt_a"].as<int>();
                int rem_b = result["vt_b"].as<int>();
                params.constraints = make_unique<VTCodeConstraints>(rem_a, rem_b);
            }
            else if (method_str == "Random")
            {
                params.method = GenerationMethod::RANDOM;
                int num_candidates = result["rand_candidates"].as<int>();
                params.constraints = make_unique<RandomConstraints>(num_candidates);
            }
            else if (method_str == "AllStrings")
            {
                params.method = GenerationMethod::ALL_STRINGS;
                params.constraints = make_unique<AllStringsConstraints>();
            }
            else if (method_str == "Diff_VTCode")
            {
                params.method = GenerationMethod::DIFFERENTIAL_VT_CODE;
                int syndrome = result["vt_synd"].as<int>();
                params.constraints = make_unique<DifferentialVTCodeConstraints>(syndrome);
            }
            else
            {
                cerr << "Error: Unknown generation method '" << method_str << "'." << endl;
                cout << options.help() << endl;
                return 1;
            }
            std::shared_ptr<CandidateGenerator> generator = CreateGenerator(params);
            generator->printInfo(cout);

            // --- Execution Loop ---
            int start_len = result["lenStart"].as<int>();
            int end_len = result["lenEnd"].as<int>();

            for (int len = start_len; len <= end_len; len++)
            {
                auto current_time = chrono::system_clock::now();
                time_t current_time_t = chrono::system_clock::to_time_t(current_time);
                tm local_tm;
#ifdef _WIN32
                localtime_s(&local_tm, &current_time_t);
#else
                localtime_r(&current_time_t, &local_tm);
#endif
                cout << "\n--- Starting Generation for Codeword Length " << len
                     << " (Current Time: " << put_time(&local_tm, "%Y-%m-%d %H:%M:%S") << ") ---" << endl;
                params.codeLen = len;

                GenerateCodebookAdj(params); // TODO: update to OOP
                // GenerateCodebookMaxClique(params);

                auto finish_time = chrono::system_clock::now();
                time_t finish_time_t = chrono::system_clock::to_time_t(finish_time);
                tm finish_tm;
#ifdef _WIN32
                localtime_s(&finish_tm, &finish_time_t);
#else
                localtime_r(&finish_time_t, &finish_tm);
#endif
                cout << "--- Finished Generation for Codeword Length " << len
                     << " (Current Time: " << put_time(&finish_tm, "%Y-%m-%d %H:%M:%S") << ") ---" << endl;
            }
        }

        // --- Completion ---
        auto end_time = chrono::steady_clock::now();
        chrono::duration<double> elapsed_secs = end_time - start_time;
        cout << fixed << setprecision(2);
        cout << "\nTotal Execution Time: " << elapsed_secs.count() << " seconds" << endl;
    }
    catch (const cxxopts::exceptions::exception &e)
    {
        cerr << "Error parsing options: " << e.what() << endl;
        cout << options.help() << endl;
        return 1;
    }
    catch (const std::exception &e)
    {
        cerr << "An unexpected error occurred: " << e.what() << endl;
        return 1;
    }

    return 0;
}

/**
 * @brief Configures the command-line parser with all available options.
 * @param options The cxxopts::Options object to configure.
 */
void configure_parser(cxxopts::Options &options)
{
    options.add_options()("h,help", "Print usage information")(
        "r,resume", "Resume generation from the save file in the specified --dir")(
        "d,dir", "Output/Resume directory name", cxxopts::value<string>()->default_value(""))
        // Core Parameters
        ("s,lenStart", "Starting codeword length", cxxopts::value<int>()->default_value("10"))(
            "e,lenEnd", "Ending codeword length", cxxopts::value<int>()->default_value("10"))(
            "D,editDist", "Minimum edit distance for the codebook", cxxopts::value<int>()->default_value("4"))
        // Biological Constraints
        ("maxRun", "Longest allowed homopolymer run", cxxopts::value<int>()->default_value("3"))(
            "minGC", "Minimum GC-content (0.0 to 1.0)", cxxopts::value<double>()->default_value("0.3"))(
            "maxGC", "Maximum GC-content (0.0 to 1.0)", cxxopts::value<double>()->default_value("0.7"))
        // Performance
        ("t,threads", "Number of threads to use", cxxopts::value<int>()->default_value("16"))(
            "saveInterval", "Interval in seconds to save progress", cxxopts::value<int>()->default_value("80000"))
        // Generation Method
        ("m,method", "Generation method: LinearCode, VTCode, Random, Diff_VTCode, AllStrings",
         cxxopts::value<string>()->default_value("LinearCode"))
        // Method-specific parameters
        ("minHD", "Min Hamming Distance for LinearCode method", cxxopts::value<int>()->default_value("3"))(
            "vt_a", "Parameter 'a' for VTCode method", cxxopts::value<int>()->default_value("0"))(
            "vt_b", "Parameter 'b' for VTCode method", cxxopts::value<int>()->default_value("0"))(
            "rand_candidates", "Number of random candidates for Random method",
            cxxopts::value<int>()->default_value("50000"))("vt_synd", "Syndrome for Differential VTCode method",
                                                           cxxopts::value<int>()->default_value("0"))
        // LinearCode vector control
        ("lc_bias_mode", "Bias vector mode: default, random, manual",
         cxxopts::value<string>()->default_value("default"))(
            "lc_row_perm_mode", "Row permutation mode: identity, random, manual",
            cxxopts::value<string>()->default_value("identity"))(
            "lc_col_perm_mode", "Column permutation mode: identity, random, manual",
            cxxopts::value<string>()->default_value("identity"))(
            "lc_bias", "Bias vector (CSV, GF(4) values 0-3)", cxxopts::value<string>())(
            "lc_row_perm", "Row permutation (CSV, 0-indexed)", cxxopts::value<string>())(
            "lc_col_perm", "Column permutation (CSV, 0-indexed)", cxxopts::value<string>())(
            "lc_random_seed", "Random seed for LinearCode vectors",
            cxxopts::value<unsigned int>()->default_value("0"));
}

/**
 * @brief Generates a timestamp string for use as a default directory name.
 * @return A string in the format "YYYY-MM-DD_HH-MM".
 */
string get_timestamp()
{
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    tm local_tm;

#ifdef _WIN32
    localtime_s(&local_tm, &now_time);
#else
    localtime_r(&now_time, &local_tm);
#endif

    stringstream ss;
    ss << put_time(&local_tm, "%Y-%m-%d_%H-%M");
    return ss.str();
}
