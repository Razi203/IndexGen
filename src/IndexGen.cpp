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
#include <vector>

// This is a header-only library for command-line argument parsing.
#include "cxxopts.hpp"
#include "json.hpp"

using json = nlohmann::json;

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
    // Capture initial working directory to resolve relative paths later
    std::filesystem::path initial_cwd = std::filesystem::current_path();

    try
    {
        configure_parser(options);
        auto result = options.parse(argc, argv);

        if (result.count("help"))
        {
            cout << options.help() << endl;
            return 0;
        }

        // --- Config Loading ---
        json config_json;
        string config_file_path;
        bool has_config = false;

        if (result.count("config")) {
            config_file_path = filesystem::absolute(result["config"].as<string>()).string();
            std::ifstream f(config_file_path);
            if (f.is_open()) {
                try {
                    f >> config_json;
                    has_config = true;
                    cout << "Loaded configuration from: " << config_file_path << endl;
                } catch (const std::exception& e) {
                    cerr << "Error parsing config JSON: " << e.what() << endl;
                    return 1;
                }
            } else {
                cerr << "Error: Config file specified but could not be opened: " << config_file_path << endl;
                return 1;
            }
        }

        // --- Helper: Parameter Resolution Strategy ---
        // 1. If CLI flag provided -> Use it (Highest Priority)
        // 2. If Config file has key at json_path -> Use it
        // 3. Fallback -> Use cxxopts Default (Lowest Priority)

        // Helper to traverse nested JSON paths safely
        auto get_json_val = [&](const vector<string>& path, auto& target) -> bool {
            if (!has_config) return false;
            json* curr = &config_json;
            for (const auto& key : path) {
                if (curr->contains(key)) {
                    curr = &((*curr)[key]);
                } else {
                    return false;
                }
            }
            try {
                target = curr->get<typename std::decay<decltype(target)>::type>();
                return true;
            } catch (...) { return false; }
        };

        auto resolve_param = [&](const string& flag, auto& target, const vector<string>& json_path = {}) {
            typedef typename std::decay<decltype(target)>::type T;

            // 1. Priority: Explicit CLI Argument
            if (result.count(flag)) {
                target = result[flag].as<T>();
                return;
            }

            // 2. Priority: Config File
            if (has_config && !json_path.empty()) {
                if (get_json_val(json_path, target)) {
                    return;
                }
            }

            // 3. Priority: CLI Default (if defined)
            try {
                target = result[flag].as<T>();
            } catch (...) {
                // No default value and not provided. Keep target as-is.
            }
        };

        // --- Directory Setup ---
        string dir_name;
        resolve_param("dir", dir_name, {"dir"});

        // Fallback for directory naming if empty
        if (dir_name.empty())
        {
            dir_name = get_timestamp();
        }

        // --- Resume Logic ---
        if (result.count("resume"))
        {
            if (dir_name.empty()) {
                cerr << "Error: When resuming, the directory with save files must be specified using --dir (or in config 'dir')." << endl;
                return 1;
            }

            filesystem::path work_dir(dir_name);
            if (!filesystem::exists(work_dir) || !filesystem::is_directory(work_dir)) {
                cerr << "Error: Directory '" << dir_name << "' not found." << endl;
                return 1;
            }

            filesystem::current_path(work_dir);
            cout << "Resuming generation in directory: " << filesystem::current_path().string() << endl;
            GenerateCodebookAdjResumeFromFile();
            return 0;
        }

        // --- New Generation Directory ---
        filesystem::path work_dir(dir_name);
        if (filesystem::exists(work_dir)) {
            cerr << "Warning: Directory '" << dir_name << "' already exists. Files may be overwritten." << endl;
        } else {
            if (!filesystem::create_directories(work_dir)) {
                cerr << "Error: Could not create directory '" << dir_name << "'." << endl;
                return 1;
            }
        }
        filesystem::current_path(work_dir);
        cout << "Output will be saved in directory: " << filesystem::current_path().string() << endl;

        // --- Parameter Configuration ---
        Params params;

        // Global / Core Mappings
        resolve_param("verify", params.verify, {"verify"});
        resolve_param("editDist", params.codeMinED, {"core", "editDist"});

        // Constraints
        resolve_param("maxRun", params.maxRun, {"constraints", "maxRun"});
        resolve_param("minGC", params.minGCCont, {"constraints", "minGC"});
        resolve_param("maxGC", params.maxGCCont, {"constraints", "maxGC"});

        // Performance
        resolve_param("threads", params.threadNum, {"performance", "threads"});
        resolve_param("saveInterval", params.saveInterval, {"performance", "saveInterval"});
        resolve_param("gpu", params.useGPU, {"performance", "use_gpu"});
        resolve_param("maxGPUMemory", params.maxGPUMemoryGB, {"performance", "max_gpu_memory_gb"});

        // Clustering
        resolve_param("cluster", params.clustering.enabled, {"clustering", "enabled"});
        resolve_param("numClusters", params.clustering.k, {"clustering", "k"});
        resolve_param("clusterVerbose", params.clustering.verbose, {"clustering", "verbose"});
        resolve_param("clusterConvergence", params.clustering.convergenceIterations, {"clustering", "convergenceIterations"}); 


        // --- Method Resolution ---
        string method_str = result["method"].as<string>();
        if (has_config && result.count("method") == 0) {
            // Check method.name
            get_json_val({"method", "name"}, method_str);
        }

        // --- Constraints Construction ---
        if (method_str == "LinearCode")
        {
            params.method = GenerationMethod::LINEAR_CODE;
            
            int minHD;
            resolve_param("minHD", minHD, {"method", "linearCode", "minHD"});

            // Resolve Modes
            string bias_mode_str, row_perm_mode_str, col_perm_mode_str;
            resolve_param("lc_bias_mode", bias_mode_str, {"method", "linearCode", "biasMode"});
            resolve_param("lc_row_perm_mode", row_perm_mode_str, {"method", "linearCode", "rowPermMode"});
            resolve_param("lc_col_perm_mode", col_perm_mode_str, {"method", "linearCode", "colPermMode"});

            VectorMode bias_mode = parseVectorMode(bias_mode_str, "bias");
            VectorMode row_perm_mode = parseVectorMode(row_perm_mode_str, "row_perm");
            VectorMode col_perm_mode = parseVectorMode(col_perm_mode_str, "col_perm");

            vector<int> bias_vec, row_perm_vec, col_perm_vec;

            // Helper to get vector from JSON array OR CLI CSV
            auto resolve_vector = [&](const string& cli_flag, const vector<string>& json_path, vector<int>& vec, const string& name) {
                // 1. Try CLI
                if (result.count(cli_flag)) {
                    vec = parseCSVVector(result[cli_flag].as<string>(), name);
                } 
                // 2. Try JSON (Array)
                else if (has_config) {
                     if (get_json_val(json_path, vec)) {
                         // Successfully loaded array
                     }
                }
            };

            // Resolve Vectors
            if (bias_mode == VectorMode::MANUAL) {
                resolve_vector("lc_bias", {"method", "linearCode", "bias"}, bias_vec, "bias");
                if (bias_vec.empty()) throw runtime_error("Manual bias vector required (via CLI --lc_bias or JSON method.linearCode.bias)");
            }

            if (row_perm_mode == VectorMode::MANUAL) {
                resolve_vector("lc_row_perm", {"method", "linearCode", "rowPerm"}, row_perm_vec, "row_perm");
                 if (row_perm_vec.empty()) throw runtime_error("Manual row permutation required");
            }

            if (col_perm_mode == VectorMode::MANUAL) {
                resolve_vector("lc_col_perm", {"method", "linearCode", "colPerm"}, col_perm_vec, "col_perm");
                if (col_perm_vec.empty()) throw runtime_error("Manual column permutation required");
            }

            unsigned int random_seed;
            resolve_param("lc_random_seed", random_seed, {"method", "linearCode", "randomSeed"});

            params.constraints = make_unique<LinearCodeConstraints>(minHD, bias_mode, row_perm_mode, col_perm_mode,
                                                                    bias_vec, row_perm_vec, col_perm_vec,
                                                                    random_seed);
        }
        else if (method_str == "VTCode")
        {
            params.method = GenerationMethod::VT_CODE;
            int rem_a, rem_b;
            resolve_param("vt_a", rem_a, {"method", "vtCode", "a"});
            resolve_param("vt_b", rem_b, {"method", "vtCode", "b"});
            params.constraints = make_unique<VTCodeConstraints>(rem_a, rem_b);
        }
        else if (method_str == "Random")
        {
            params.method = GenerationMethod::RANDOM;
            int num_candidates;
            resolve_param("rand_candidates", num_candidates, {"method", "random", "candidates"});
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
            int syndrome;
            resolve_param("vt_synd", syndrome, {"method", "diffVtCode", "syndrome"});
            params.constraints = make_unique<DifferentialVTCodeConstraints>(syndrome);
        }
        else if (method_str == "FileRead")
        {
            params.method = GenerationMethod::FILE_READ;
            string input_file;
            resolve_param("input_file", input_file, {"method", "fileRead", "input_file"});
            
            if (input_file.empty()) {
                throw std::runtime_error("--input_file (or method.fileRead.input_file) required when method=FileRead");
            }
            
            // Resolve relative paths against the initial working directory
            // because the program changes current_path to the output directory earlier.
            filesystem::path p(input_file);
            if (p.is_relative()) {
                input_file = (initial_cwd / p).string();
                cout << "Resolved input file path to: " << input_file << endl;
            }

            params.constraints = make_unique<FileReadConstraints>(input_file);
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
        int start_len, end_len;
        resolve_param("lenStart", start_len, {"core", "lenStart"});
        resolve_param("lenEnd", end_len, {"core", "lenEnd"});

        auto start_time = chrono::steady_clock::now();

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

            GenerateCodebookAdj(params); 
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
            "saveInterval", "Interval in seconds to save progress", cxxopts::value<int>()->default_value("80000"))(
            "verify", "Verify the codebook distance after generation", cxxopts::value<bool>()->default_value("false"))(
            "gpu", "Use GPU for adjacency list generation", cxxopts::value<bool>()->default_value("true"))(
            "maxGPUMemory", "Maximum GPU memory to use in GB", cxxopts::value<double>()->default_value("10.0"))(
            "c,config", "JSON configuration file", cxxopts::value<string>())
            ("cluster", "Use cluster-based iterative solving", cxxopts::value<bool>()->default_value("false"))
            ("numClusters", "Number of clusters to partition candidates into", cxxopts::value<int>()->default_value("500"))
            ("clusterVerbose", "Verbose clustering output", cxxopts::value<bool>()->default_value("false"))
            ("clusterConvergence", "Number of identical iterations for convergence", cxxopts::value<int>()->default_value("3"))
        // Generation Method
        ("m,method", "Generation method: LinearCode, VTCode, Random, Diff_VTCode, AllStrings, FileRead",
         cxxopts::value<string>()->default_value("LinearCode"))
        // Method-specific parameters
        ("input_file", "Input file for FileRead method", cxxopts::value<string>())(
            "minHD", "Min Hamming Distance for LinearCode method", cxxopts::value<int>()->default_value("3"))(
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