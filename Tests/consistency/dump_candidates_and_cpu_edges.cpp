#include "CandidateGenerator.hpp"
#include "EditDistance.hpp"
#include "IndexGen.hpp"
#include "Utils.hpp"
#include "json.hpp" // json.hpp is in include/
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <algorithm>
#include <chrono>

using namespace std;
using json = nlohmann::json;

// Helper to write binary edges (i, j) where i < j
void write_edges_bin(const std::string& filename, const std::vector<std::pair<int, int>>& edges) {
    std::ofstream out(filename, std::ios::binary);
    // Buffer for efficiency
    std::vector<int32_t> buffer;
    buffer.reserve(10000); 

    for (const auto& kv : edges) {
        buffer.push_back(kv.first);
        buffer.push_back(kv.second);
        if (buffer.size() >= 10000) {
            out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(int32_t));
            buffer.clear();
        }
    }
    if (!buffer.empty()) {
        out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(int32_t));
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    std::string config_path = argv[1];
    Params params;
    try {
        LoadParamsFromJson(params, config_path);
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        return 1;
    }

    // Override params for consistent generation if needed, but LoadParamsFromJson should handle it.
    // IndexGen loop:
    // int start_len = result["lenStart"].as<int>();
    // ... we need to replicate this logic if config has ranges.
    
    // Check config for core.lenStart
    std::ifstream f(config_path);
    json j;
    f >> j;
    int lenStart = 10;
    int lenEnd = 10;
    if (j.contains("core")) {
        lenStart = j["core"].value("lenStart", 10);
        lenEnd = j["core"].value("lenEnd", 10);
    }

    // We will just process the FIRST length if there is a range, 
    // or we can iterate. The user asked for "the configuration in config.json". 
    // config.json has lenStart=11, lenEnd=11.
    // So we iterate.
    
    for (int len = lenStart; len <= lenEnd; ++len) {
        std::cout << "Processing length: " << len << std::endl;
        params.codeLen = len;
        
        // 1. Generate Candidates
        auto start = std::chrono::steady_clock::now();
        std::cout << "Generating candidates..." << std::endl;
        // Use the factory to create generator
        std::shared_ptr<CandidateGenerator> generator = CreateGenerator(params);
        std::vector<std::string> candidates = generator->generate();
        candidates = generator->applyFilters(candidates);

        std::cout << "Generated " << candidates.size() << " candidates." << std::endl;
        
        // 2. Dump Candidates to candidates.txt
        std::string candFile = "tests/candidates.txt";
        {
            std::ofstream out(candFile);
            for (const auto& s : candidates) {
                out << s << "\n";
            }
        }
        std::cout << "Saved candidates to " << candFile << std::endl;
        
        // 3. Compute CPU Edges (i < j ONLY)
        std::vector<std::pair<int, int>> edges;
        std::mutex edges_mutex;
        
        int minED = params.codeMinED;
        int threadNum = params.threadNum;
        if (threadNum <= 0) threadNum = 1;

        std::cout << "Computing edges with " << threadNum << " threads (minED=" << minED << ")..." << std::endl;
        
        auto compute_chunk = [&](int start_idx, int end_idx) {
             std::vector<std::pair<int, int>> local_edges;
             for (int i = start_idx; i < end_idx; ++i) {
                 PatternHandle H = MakePattern(candidates[i]);
                 for (int j = i + 1; j < (int)candidates.size(); ++j) {
                     // logic from SparseMat.cpp: FillAdjListTH
                     // EditDistanceBandedAtLeast
                     bool distAtLeast = EditDistanceBandedAtLeast(candidates[j], H, minED);
                     if (!distAtLeast) { // Distance < minED
                         local_edges.push_back({i, j});
                     }
                 }
             }
             std::lock_guard<std::mutex> lock(edges_mutex);
             edges.insert(edges.end(), local_edges.begin(), local_edges.end());
        };
        
        std::vector<std::thread> threads;
        int n = candidates.size();
        int chunk_size = (n + threadNum - 1) / threadNum;
        
        for (int t = 0; t < threadNum; ++t) {
            int start_idx = t * chunk_size;
            int end_idx = std::min(start_idx + chunk_size, n);
            if (start_idx < end_idx) {
                threads.emplace_back(compute_chunk, start_idx, end_idx);
            }
        }
        
        for (auto& th : threads) th.join();
        
        std::cout << "Computed " << edges.size() << " edges." << std::endl;
        
        // 4. Sort Edges to ensure determinism for comparison
        std::sort(edges.begin(), edges.end());
        
        // 5. Write CPU edges
        std::string cpuEdgesFile = "tests/cpu_edges.bin";
        write_edges_bin(cpuEdgesFile, edges);
        std::cout << "Saved CPU edges to " << cpuEdgesFile << std::endl;
        
        auto end = std::chrono::steady_clock::now();
        std::cout << "Total time: " << std::chrono::duration<double>(end - start).count() << "s" << std::endl;
    }

    return 0;
}
