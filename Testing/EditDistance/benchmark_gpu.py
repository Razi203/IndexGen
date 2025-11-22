import os
import time
import ctypes
import numpy as np
from numba import cuda, uint64, int32, void

# -------------------- 0. SETUP & COMPILATION --------------------
# We assume EditDistance.hpp exists. We compile bridge.cpp into a shared library.
print(">>> Compiling C++ bridge...")
os.system("g++ -O3 -shared -fPIC -std=c++17 bridge.cpp -o libeditdist.so")

# Load the C++ library
try:
    cpp_lib = ctypes.CDLL("./libeditdist.so")
    cpp_lib.benchmark_c_batch.argtypes = [
        ctypes.c_char_p,                # query
        ctypes.c_char_p,                # flat candidates
        ctypes.c_int,                   # num candidates
        ctypes.c_int,                   # len
        np.ctypeslib.ndpointer(dtype=np.int32, ndim=1, flags='C_CONTIGUOUS') # results
    ]
    print(">>> C++ Library loaded successfully.")
except Exception as e:
    print(f"Error loading C++ library: {e}")
    print("Ensure 'bridge.cpp' and 'EditDistance.hpp' are in the same folder.")
    exit(1)

# -------------------- 1. NUMBA CUDA KERNEL --------------------
# Bit-Parallel Myers for N <= 64 (Single Word)
# We assume alphabet is mapped to 0, 1, 2, 3.
@cuda.jit
def myers_gpu_kernel_64(encoded_candidates, peq_table, results, m):
    # grid setup
    idx = cuda.grid(1)
    if idx >= encoded_candidates.shape[0]:
        return

    # Initialize Bit-Vectors
    pv = uint64(~0)
    mv = uint64(0)
    score = int32(m)
    
    # Loop over the vector length (N)
    # candidate data is shape [num_candidates, len]
    for i in range(m):
        # Fetch encoded char (0-3)
        char_code = encoded_candidates[idx, i]
        
        # Get Precomputed Equation (Eq) for this character
        # peq_table is shape [4] containing bitmasks
        eq = peq_table[char_code]
        
        # --- MYERS LOGIC (Matches C++ MyersSingleWord) ---
        xv = eq | mv
        
        # D0 = (((X & PV) + PV) ^ PV) | X;
        # Note: Python integers are infinite precision, but Numba uint64 wraps.
        # We must ensure wrapping behavior for addition.
        # (xv & pv) + pv might overflow 64 bits. 
        
        t1 = xv & pv
        t2 = t1 + pv # This add can carry.
        
        d0 = ((t2 ^ pv) | xv)
        
        hn = pv & d0
        hp = mv | ~(pv | d0)
        
        x2 = (hp << 1) | 1
        mv = x2 & d0
        pv = (hn << 1) | ~(x2 | d0)
        
        # Update Score
        # highest_bit check: (m-1)
        bit_pos = m - 1
        is_hp_set = (hp >> bit_pos) & 1
        is_hn_set = (hn >> bit_pos) & 1
        
        score += is_hp_set
        score -= is_hn_set
        
    results[idx] = score

# -------------------- 2. HELPER FUNCTIONS --------------------

def encode_seq(seq_str):
    """Map ACGT -> 0,1,2,3"""
    mapping = {'A': 0, 'C': 1, 'G': 2, 'T': 3}
    return [mapping[c] for c in seq_str]

def build_peq_cpu(query_str):
    """Build the PEQ table (4x1 uint64) for the GPU"""
    m = len(query_str)
    # 4-ary alphabet
    table = np.zeros(4, dtype=np.uint64)
    encoded = encode_seq(query_str)
    
    for i, char_code in enumerate(encoded):
        # Set the i-th bit for this char
        table[char_code] |= np.uint64(1 << i)
        
    return table

def generate_data(n_candidates, length):
    print(f"\nGenerating {n_candidates} vectors of length {length}...")
    # Create random DNA strings
    bases = np.array(['A', 'C', 'G', 'T'])
    # CPU generation of strings
    data = np.random.choice(bases, size=(n_candidates, length))
    
    # For C++: flatten to one big bytes string
    # (We join rows then join chars, simplified)
    flat_str = b"".join([row.tobytes() for row in data])
    
    # For GPU: Encoded int8 array
    mapping = {'A': 0, 'C': 1, 'G': 2, 'T': 3}
    # Fast vectorization
    encoded = np.zeros(data.shape, dtype=np.int8)
    for b, val in mapping.items():
        encoded[data == b] = val
        
    query_arr = np.random.choice(bases, size=length)
    query_str = "".join(query_arr)
    query_bytes = query_str.encode('utf-8')
    
    return query_str, query_bytes, flat_str, encoded

# -------------------- 3. MAIN BENCHMARK --------------------
def run_benchmark():
    # Params
    LENGTH = 64  # Bit-parallel sweet spot
    N_CANDIDATES = 1_000_000 # 1 Million comparisons
    
    query_str, query_bytes, c_flat_candidates, gpu_encoded_candidates = generate_data(N_CANDIDATES, LENGTH)
    
    # --- A. C++ RUN ---
    print(f"Running C++ Benchmark on {N_CANDIDATES} items...")
    c_results = np.zeros(N_CANDIDATES, dtype=np.int32)
    
    start_c = time.time()
    cpp_lib.benchmark_c_batch(
        query_bytes, 
        c_flat_candidates, 
        N_CANDIDATES, 
        LENGTH, 
        c_results
    )
    end_c = time.time()
    time_c = end_c - start_c
    print(f"C++ Time: {time_c:.4f}s")

    # --- B. GPU RUN ---
    print(f"Running GPU Benchmark on {N_CANDIDATES} items...")
    
    # 1. Transfer data to GPU
    d_candidates = cuda.to_device(gpu_encoded_candidates)
    d_results = cuda.device_array(N_CANDIDATES, dtype=np.int32)
    
    # 2. Precompute Query PEQ on CPU
    peq = build_peq_cpu(query_str)
    d_peq = cuda.to_device(peq)
    
    # 3. Run Kernel
    threads_per_block = 256
    blocks = (N_CANDIDATES + threads_per_block - 1) // threads_per_block
    
    cuda.synchronize()
    start_g = time.time()
    
    myers_gpu_kernel_64[blocks, threads_per_block](d_candidates, d_peq, d_results, LENGTH)
    
    cuda.synchronize() # Wait for GPU to finish
    end_g = time.time()
    
    gpu_results = d_results.copy_to_host()
    time_g = end_g - start_g
    
    print(f"GPU Time: {time_g:.4f}s (Includes Kernel exec only)")
    print(f"GPU Speedup: {time_c / time_g:.2f}x")

    # --- C. VALIDATION ---
    print("\nValidating results...")
    mismatches = np.sum(c_results != gpu_results)
    if mismatches == 0:
        print("✅ SUCCESS: C++ and GPU results match perfectly!")
    else:
        print(f"❌ FAILURE: {mismatches} mismatches found.")
        # Debug print first mismatch
        for i in range(N_CANDIDATES):
            if c_results[i] != gpu_results[i]:
                print(f"Index {i}: C++ says {c_results[i]}, GPU says {gpu_results[i]}")
                break

if __name__ == "__main__":
    run_benchmark()