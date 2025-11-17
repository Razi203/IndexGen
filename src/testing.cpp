/**
 * @file testing.cpp
 * @brief A standalone testing program for candidate generation and adjacency list computation.
 */

#include "CandidateGenerator.hpp"
#include "Candidates.hpp"
#include "EditDistance.hpp"
#include "Utils.hpp"
#include "cxxopts.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <omp.h>
#include <unordered_set>

using namespace std;

/**
 * @brief Configures the command-line parser with all available options.
 */
void configure_parser(cxxopts::Options &options)
{
    options.add_options()("h,help", "Print usage information")
        // Core Parameters
        ("n,length", "Codeword length", cxxopts::value<int>()->default_value("21"))(
            "D,editDist", "Minimum edit distance for the codebook", cxxopts::value<int>()->default_value("3"))
        // Biological Constraints
        ("maxRun", "Longest allowed homopolymer run", cxxopts::value<int>()->default_value("3"))(
            "minGC", "Minimum GC-content (0.0 to 1.0)", cxxopts::value<double>()->default_value("0.3"))(
            "maxGC", "Maximum GC-content (0.0 to 1.0)", cxxopts::value<double>()->default_value("0.7"))
        // Parallelization
        ("t,threads", "Number of threads for adjacency list computation", cxxopts::value<int>()->default_value("32"))
        // Generation Method
        ("m,method", "Generation method: LinearCode, VTCode, Random, Diff_VTCode, AllStrings, RandomLinear",
         cxxopts::value<string>()->default_value("LinearCode"))
        // Method-specific parameters
        ("minHD", "Min Hamming Distance for LinearCode method", cxxopts::value<int>()->default_value("3"))(
            "vt_a", "Parameter 'a' for VTCode method", cxxopts::value<int>()->default_value("0"))(
            "vt_b", "Parameter 'b' for VTCode method", cxxopts::value<int>()->default_value("0"))(
            "rand_candidates", "Number of random candidates for Random method",
            cxxopts::value<int>()->default_value("50000"))("vt_synd", "Syndrome for Differential VTCode method",
                                                           cxxopts::value<int>()->default_value("0"))(
            "randlin_minHD", "Min Hamming Distance for RandomLinear method", cxxopts::value<int>()->default_value("3"))(
            "randlin_candidates", "Number of random candidates for RandomLinear method",
            cxxopts::value<int>()->default_value("50000"));
}

int main(int argc, char *argv[])
{
    cxxopts::Options options("Testing", "Candidate generation and adjacency list computation test");

    try
    {
        configure_parser(options);
        auto result = options.parse(argc, argv);

        if (result.count("help"))
        {
            cout << options.help() << endl;
            return 0;
        }

        // Parse core parameters
        int code_length = result["length"].as<int>();
        int min_edit_dist = result["editDist"].as<int>();
        int num_threads = result["threads"].as<int>();

        // Validate input
        if (code_length < 1)
        {
            cerr << "Error: Code length must be at least 1." << endl;
            return 1;
        }

        if (min_edit_dist < 1)
        {
            cerr << "Error: Minimum edit distance must be at least 1." << endl;
            return 1;
        }

        // Setup parameters structure
        Params params;
        params.codeLen = code_length;
        params.codeMinED = min_edit_dist;
        params.maxRun = result["maxRun"].as<int>();
        params.minGCCont = result["minGC"].as<double>();
        params.maxGCCont = result["maxGC"].as<double>();
        params.threadNum = num_threads;
        params.saveInterval = 0; // Not used in testing

        // Parse generation method
        string method_str = result["method"].as<string>();

        if (method_str == "LinearCode")
        {
            params.method = GenerationMethod::LINEAR_CODE;
            int minHD = result["minHD"].as<int>();
            params.constraints = make_unique<LinearCodeConstraints>(minHD);
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
        else if (method_str == "RandomLinear")
        {
            params.method = GenerationMethod::RANDOM_LINEAR;
            int minHD = result["randlin_minHD"].as<int>();
            int num_candidates = result["randlin_candidates"].as<int>();
            params.constraints = make_unique<RandomLinearConstraints>(minHD, num_candidates);
        }
        else
        {
            cerr << "Error: Unknown generation method '" << method_str << "'." << endl;
            cout << options.help() << endl;
            return 1;
        }

        // Print header
        cout << "==============================================\n";
        cout << "Candidate Generation & Adjacency Test\n";
        cout << "==============================================\n";
        cout << "Parameters:\n";
        cout << "  Code Length (n):              " << code_length << endl;
        cout << "  Min Edit Distance:            " << min_edit_dist << endl;
        cout << "  Max Homopolymer Run:          " << params.maxRun << endl;
        cout << "  GC-Content Range:             " << params.minGCCont << " - " << params.maxGCCont << endl;
        cout << "  Threads (adjacency):          " << num_threads << endl;
        cout << "----------------------------------------------\n";

        // Create generator and print info
        shared_ptr<CandidateGenerator> generator = CreateGenerator(params);
        generator->printInfo(cout);
        cout << "----------------------------------------------\n";

        // Generate candidates
        cout << "Generating candidates..." << endl;
        vector<string> unfiltered = generator->generate();
        cout << "  Unfiltered candidates:        " << unfiltered.size() << endl;

        // Apply filters
        cout << "Applying filters..." << endl;
        vector<string> candidates = generator->applyFilters(unfiltered);
        cout << "  Filtered candidates:          " << candidates.size() << endl;

        // Save candidates to file
        cout << "----------------------------------------------\n";
        cout << "Saving candidates to 'candidates.txt'..." << endl;
        ofstream candidates_file("candidates.txt");
        if (!candidates_file.is_open())
        {
            cerr << "Error: Could not open 'candidates.txt' for writing." << endl;
            return 1;
        }
        for (const auto &candidate : candidates)
        {
            candidates_file << candidate << "\n";
        }
        candidates_file.close();
        cout << "Candidates saved successfully!" << endl;

        // Build adjacency list based on edit distance
        cout << "----------------------------------------------\n";
        cout << "Computing adjacency list (edit distance < " << min_edit_dist << ")..." << endl;

        int num_nodes = candidates.size();
        vector<unordered_set<int>> adj_list(num_nodes);
        vector<mutex> adj_list_mutexes(num_nodes);
        long long int edge_count = 0;

        // Precompute pattern handles for efficiency
        vector<PatternHandle> pattern_handles;
        pattern_handles.reserve(num_nodes);
        for (const auto &candidate : candidates)
        {
            pattern_handles.push_back(MakePattern(candidate));
        }

        // Set number of threads
        omp_set_num_threads(num_threads);

        // Build adjacency list in parallel
#pragma omp parallel for schedule(dynamic) reduction(+ : edge_count)
        for (int i = 0; i < num_nodes; ++i)
        {
            for (int j = i + 1; j < num_nodes; ++j)
            {
                // Check if edit distance is less than min_edit_dist
                // If so, add an edge (they conflict)
                int ed = EditDistanceBanded(candidates[j], pattern_handles[i], min_edit_dist);
                if (ed < min_edit_dist)
                {
                    // Thread-safe insertion using mutexes
                    adj_list_mutexes[i].lock();
                    adj_list[i].insert(j);
                    adj_list_mutexes[i].unlock();

                    adj_list_mutexes[j].lock();
                    adj_list[j].insert(i);
                    adj_list_mutexes[j].unlock();

                    edge_count++;
                }
            }
        }

        cout << "Adjacency list computed!" << endl;
        cout << "  Number of Edges:              " << edge_count << endl;

        // Save graph to file in METIS/KaMIS format
        cout << "----------------------------------------------\n";
        cout << "Saving graph to 'graph.txt'..." << endl;
        ofstream graph_file("graph.txt");
        if (!graph_file.is_open())
        {
            cerr << "Error: Could not open 'graph.txt' for writing." << endl;
            return 1;
        }

        // First line: number of nodes and number of edges
        graph_file << num_nodes << " " << edge_count << "\n";

        // Each subsequent line i contains the neighbors of vertex i (1-indexed)
        for (int i = 0; i < num_nodes; ++i)
        {
            // Convert neighbors to 1-indexed and sort them
            vector<int> neighbors(adj_list[i].begin(), adj_list[i].end());
            sort(neighbors.begin(), neighbors.end());

            for (size_t k = 0; k < neighbors.size(); ++k)
            {
                if (k > 0)
                    graph_file << " ";
                graph_file << (neighbors[k] + 1); // Convert to 1-indexed
            }
            graph_file << "\n";
        }
        graph_file.close();
        cout << "Graph saved successfully!" << endl;
        cout << "==============================================\n";
    }
    catch (const cxxopts::exceptions::exception &e)
    {
        cerr << "Error parsing options: " << e.what() << endl;
        cout << options.help() << endl;
        return 1;
    }
    catch (const exception &e)
    {
        cerr << "An unexpected error occurred: " << e.what() << endl;
        return 1;
    }

    return 0;
}