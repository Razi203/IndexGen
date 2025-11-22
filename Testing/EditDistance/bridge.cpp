#include "EditDistance.hpp"
#include <vector>
#include <cstring>

// extern "C" prevents C++ name mangling so Python ctypes can find the function
extern "C" {

    // Batch compute: One Query vs Many Candidates
    // This ensures we benchmark the algorithm, not the Python <-> C++ function call overhead
    void benchmark_c_batch(const char* query_ptr, 
                           const char* flat_candidates, 
                           int num_candidates, 
                           int len, 
                           int* results) 
    {
        std::string query(query_ptr, len);
        
        // Precompute pattern once (fair comparison)
        PatternHandle H(query);
        
        for (int i = 0; i < num_candidates; ++i) {
            // Construct string view from flat buffer
            // Note: In a real highly-optimized C++ engine, we would avoid std::string allocs here,
            // but standard Myers implementation usually takes string/char*.
            // For max speed, we use the pointer directly if possible, 
            // but your HPP takes std::string. We'll wrap efficiently.
            std::string cand(flat_candidates + (i * len), len);
            
            // Call the exact version
            results[i] = (H.m <= 64) ? MyersSingleWord(H, cand) : MyersMultiWord(H, cand);
        }
    }
}