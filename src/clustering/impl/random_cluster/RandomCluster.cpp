#include "RandomCluster.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <random>
#include <numeric>
#include <filesystem>
#include <algorithm>
#include <cstdint>

namespace indexgen {
namespace clustering {
namespace impl {

namespace fs = std::filesystem;

static std::string getProjectRoot() {
    try {
        fs::path exe_path = fs::read_symlink("/proc/self/exe");
        fs::path exe_dir = exe_path.parent_path();
        fs::path search_path = exe_dir;
        for (int i = 0; i < 5; ++i) {
            fs::path src_dir = search_path / "src";
            if (fs::exists(src_dir) && fs::is_directory(src_dir)) {
                if (fs::exists(src_dir / "gpu_cluster.py")) {
                    return search_path.string();
                }
            }
            if (search_path.has_parent_path() && search_path.parent_path() != search_path) {
                search_path = search_path.parent_path();
            } else {
                break;
            }
        }
    } catch (const std::exception& e) {
    }
    return ".";
}

RandomCluster::RandomCluster(int k) : k(k) {}

std::vector<std::vector<std::string>> RandomCluster::fit(const std::vector<std::string>& data) {
    if (data.empty()) return {};
    if (k <= 0) return {data};

    int actual_k = std::min(k, (int)data.size());

    // 1. Select N (actual_k) random centers
    std::vector<int> indices(data.size());
    std::iota(indices.begin(), indices.end(), 0);
    
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(indices.begin(), indices.end(), g);
    
    std::vector<std::string> centers;
    centers.reserve(actual_k);
    for(int i = 0; i < actual_k; ++i) {
        centers.push_back(data[indices[i]]);
    }

    // Write temp files
    std::string vectors_file = "temp_rc_vectors.txt";
    std::string centers_file = "temp_rc_centers.txt";
    std::string assignments_file = "temp_rc_assignments.bin";

    {
        std::ofstream v_out(vectors_file);
        for(const auto& s : data) v_out << s << "\n";
    }
    {
        std::ofstream c_out(centers_file);
        for(const auto& s : centers) c_out << s << "\n";
    }

    // Call python script
    const char* env_root = std::getenv("INDEXGEN_ROOT");
    std::string project_root = (env_root) ? std::string(env_root) : getProjectRoot();
    std::string script_path = project_root + "/src/gpu_cluster.py";
    
    std::string cmd = "python " + script_path + " " + vectors_file + " " + centers_file + " " + assignments_file + " 32768";
    std::cout << "[C++ RandomCluster] Executing GPU Cluster: " << cmd << std::endl;
    int ret = system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "Error: Python GPU cluster script failed." << std::endl;
        exit(1);
    }

    // Read assignments
    std::ifstream a_in(assignments_file, std::ios::binary | std::ios::ate);
    if (!a_in.is_open()) {
        std::cerr << "Error: Could not open assignments file." << std::endl;
        exit(1);
    }
    
    std::streamsize size = a_in.tellg();
    a_in.seekg(0, std::ios::beg);
    
    int expected_size = data.size() * sizeof(int32_t);
    if (size != expected_size) {
        std::cerr << "Error: Assignments file size mismatch. Expected " << expected_size << ", got " << size << std::endl;
        exit(1);
    }
    
    std::vector<int32_t> assignments(data.size());
    if (a_in.read(reinterpret_cast<char*>(assignments.data()), size)) {
        // Build clusters
        std::vector<std::vector<std::string>> clusters(actual_k);
        for(size_t i = 0; i < data.size(); ++i) {
            int32_t c_idx = assignments[i];
            if (c_idx >= 0 && c_idx < actual_k) {
                clusters[c_idx].push_back(data[i]);
            } else {
                clusters[0].push_back(data[i]);
            }
        }
        
        // Clean up
        remove(vectors_file.c_str());
        remove(centers_file.c_str());
        remove(assignments_file.c_str());
        
        // Remove empty clusters
        std::vector<std::vector<std::string>> final_clusters;
        for (auto& c : clusters) {
            if (!c.empty()) {
                final_clusters.push_back(std::move(c));
            }
        }
        
        return final_clusters;
    } else {
        std::cerr << "Error: Failed to read assignments file." << std::endl;
        exit(1);
    }
}

} // namespace impl
} // namespace clustering
} // namespace indexgen
