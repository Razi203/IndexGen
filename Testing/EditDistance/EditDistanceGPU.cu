#include "EditDistanceGPU.cuh"
#include <algorithm>
#include <cuda_runtime.h>
#include <iostream>

// -------------------- CUDA KERNEL --------------------

// Constant memory for the Peq table (Pattern Equation).
// 256 entries (ASCII) * 1 block (uint64) covers patterns up to length 64.
__constant__ uint64_t d_peq_table[256];

__global__ void myers_kernel_64(const char *candidates, int *results, int num_candidates, int m)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= num_candidates)
        return;

    // Initialize Bit-Vectors (Same as CPU MyersSingleWord)
    uint64_t pv = ~0ULL;
    uint64_t mv = 0ULL;
    int score = m;

    // Pointer to this thread's candidate string in the flat array
    const char *my_cand = candidates + (size_t)idx * m;

    // Loop over the string length
    // #pragma unroll // Optional: Unrolling can help if m is compile-time constant
    for (int i = 0; i < m; ++i)
    {
        // 1. Read character
        unsigned char c = (unsigned char)my_cand[i];

        // 2. Fetch precomputed Eq from Constant Memory
        uint64_t eq = d_peq_table[c];

        // 3. Bit-Parallel Myers Step
        uint64_t xv = eq | mv;
        uint64_t x = (xv & pv) + pv;
        uint64_t d0 = ((x ^ pv) | xv);

        uint64_t hn = pv & d0;
        uint64_t hp = mv | ~(pv | d0);

        uint64_t x2 = (hp << 1) | 1ULL;
        mv = x2 & d0;
        pv = (hn << 1) | ~(x2 | d0);

        // 4. Update Score (Branchless)
        // Extract (m-1)-th bit
        int shift = m - 1;
        score += (int)((hp >> shift) & 1) - (int)((hn >> shift) & 1);
    }

    results[idx] = score;
}

// -------------------- HOST HELPER FUNCTIONS --------------------

void checkCuda(cudaError_t result, char const *const func, const char *const file, int const line)
{
    if (result)
    {
        std::cerr << "CUDA error at " << file << ":" << line << " code=" << result << " \"" << func << "\" \n";
        exit(99);
    }
}

void BuildPeqHost(const std::string &pattern, uint64_t *table)
{
    int m = pattern.size();
    std::fill(table, table + 256, 0ULL);
    for (int i = 0; i < m; ++i)
    {
        unsigned char c = (unsigned char)pattern[i];
        table[c] |= (1ULL << i);
    }
}

// -------------------- MAIN WRAPPER --------------------

float ComputeEditDistanceBatchGPU(const std::string &query, const std::vector<char> &flat_candidates,
                                  int num_candidates, int len, int *h_results)
{
    if (len > 64)
    {
        std::cerr << "Error: This GPU kernel only supports strings <= 64 chars.\n";
        return 0.0f;
    }

    size_t cand_bytes = flat_candidates.size() * sizeof(char);
    size_t res_bytes = num_candidates * sizeof(int);

    // 1. Device Allocation
    char *d_candidates = nullptr;
    int *d_results = nullptr;

    checkCudaErrors(cudaMalloc(&d_candidates, cand_bytes));
    checkCudaErrors(cudaMalloc(&d_results, res_bytes));

    // 2. Prepare Peq Table and Copy to Constant Memory
    uint64_t h_peq[256];
    BuildPeqHost(query, h_peq);
    checkCudaErrors(cudaMemcpyToSymbol(d_peq_table, h_peq, sizeof(h_peq)));

    // 3. Copy Candidates to Device
    //    Note: For max perf, you should keep data on GPU between queries if possible.
    checkCudaErrors(cudaMemcpy(d_candidates, flat_candidates.data(), cand_bytes, cudaMemcpyHostToDevice));

    // 4. Launch Configuration
    int threads = 256;
    int blocks = (num_candidates + threads - 1) / threads;

    // 5. Run & Time
    cudaEvent_t start, stop;
    checkCudaErrors(cudaEventCreate(&start));
    checkCudaErrors(cudaEventCreate(&stop));

    checkCudaErrors(cudaEventRecord(start));
    myers_gpu_kernel_64<<<blocks, threads>>>(d_candidates, d_results, num_candidates, len);
    checkCudaErrors(cudaEventRecord(stop));
    checkCudaErrors(cudaEventSynchronize(stop));

    float milliseconds = 0;
    checkCudaErrors(cudaEventElapsedTime(&milliseconds, start, stop));

    // 6. Copy Results Back
    checkCudaErrors(cudaMemcpy(h_results, d_results, res_bytes, cudaMemcpyDeviceToHost));

    // 7. Cleanup
    checkCudaErrors(cudaFree(d_candidates));
    checkCudaErrors(cudaFree(d_results));
    checkCudaErrors(cudaEventDestroy(start));
    checkCudaErrors(cudaEventDestroy(stop));

    return milliseconds;
}