#pragma once
#include <cstdint>
#include <string>
#include <vector>

// -------------------- GPU API --------------------

// Computes Edit Distance for a single query against a massive batch of candidates.
//
// Params:
//   query: The search string (pattern).
//   flat_candidates: A single flat vector containing all candidate strings concatenated.
//                    (e.g., if strings are length 64, index i starts at i*64).
//   num_candidates: How many strings are in the batch.
//   len: Length of each string (must be <= 64 for this optimized kernel).
//   results: Pointer to host memory where results will be stored (size = num_candidates).
//
// Returns: Time taken (in milliseconds) for the GPU execution.
float ComputeEditDistanceBatchGPU(const std::string &query, const std::vector<char> &flat_candidates,
                                  int num_candidates, int len, int *results);

// Helper to check CUDA errors
void checkCuda(cudaError_t result, char const *const func, const char *const file, int const line);
#define checkCudaErrors(val) checkCuda((val), #val, __FILE__, __LINE__)