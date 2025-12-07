#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include "EditDistance.hpp" 

// Function to split a string by delimiter
std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    size_t start = 0, end = 0;
    while ((end = s.find(delimiter, start)) != std::string::npos) {
        tokens.push_back(s.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(s.substr(start));
    return tokens;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <data_file> <threshold>" << std::endl;
        return 1;
    }

    std::string filepath = argv[1];
    int threshold = std::stoi(argv[2]);

    // 1. Load Data
    std::ifstream infile(filepath);
    if (!infile.is_open()) {
        std::cerr << "Error opening file." << std::endl;
        return 1;
    }

    std::vector<std::string> strings;
    std::string line;
    while (std::getline(infile, line)) {
        if (!line.empty()) {
            // Remove any potential carriage return if file was made on Windows
            if (line.back() == '\r') line.pop_back();
            strings.push_back(line);
        }
    }
    infile.close();

    int N = strings.size();
    std::cout << "Loaded " << N << " strings (Format 0123). Computing adjacency with Threshold < " << threshold << "..." << std::endl;

    // 2. Compute Edges (Memory Optimized: No Matrix Storage)
    long long edge_count = 0;

    auto start = std::chrono::high_resolution_clock::now();
    
    // Parallel loop with reduction on edge_count
    #pragma omp parallel for schedule(dynamic) reduction(+:edge_count)
    for (int i = 0; i < N; ++i) {
        // Pre-build pattern handle for row i
        // EditDistance.hpp handles ASCII '0', '1', '2', '3' generically.
        PatternHandle H(strings[i]);

        for (int j = i + 1; j < N; ++j) {
            // Use Banded Edit Distance for speed
            int k = threshold - 1;
            int dist = EditDistanceBanded(strings[j], H, k);
            
            if (dist < threshold) {
                edge_count++;
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    // 3. Output Stats
    std::cout << "CPP_TIME: " << elapsed.count() << " seconds" << std::endl;
    std::cout << "CPP_EDGES: " << edge_count << std::endl;

    return 0;
}