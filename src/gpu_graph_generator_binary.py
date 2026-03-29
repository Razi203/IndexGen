import sys
import os
import numpy as np
import time
from cuda_edit_distance_binary import solve_edit_distance_cuda_binary

# Configuration
# Chunk size for writing binary data to keep RAM low
WRITE_BUFFER_SIZE = 10_000_000 

def load_candidates(filename):
    with open(filename, 'r') as f:
        # Load and strip whitespace
        strings = [line.strip() for line in f if line.strip()]
    return strings

def main():
    if len(sys.argv) < 4:
        print("Usage: python gpu_graph_generator_binary.py <input_vectors.txt> <output_edges.bin> <threshold> [max_gpu_memory_gb]")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]
    threshold = int(sys.argv[3])
    
    max_gpu_gb = 9.0

    if len(sys.argv) > 4:
        max_gpu_gb = float(sys.argv[4])
    
    # 1. Load Strings
    print(f"[Python GPU] Loading strings from {input_file}...")
    strings = load_candidates(input_file)
    N = len(strings)
    print(f"[Python GPU] Loaded {N} binary strings. Running CUDA Solver with T={threshold}, Memory Limit={max_gpu_gb}GB...")

    # 2. Run Optimized CUDA Kernel
    print(f"[Python GPU] Writing edges directly from CUDA to binary file {output_file}...")
    
    # Pre-clear the output file to prepare for appending
    open(output_file, 'wb').close()

    start_t = time.time()
    try:
        edge_count = solve_edit_distance_cuda_binary(strings, threshold, output_file, max_gpu_memory_gb=max_gpu_gb)
    except Exception as e:
        print(f"[Error] CUDA computation failed: {e}")
        sys.exit(1)
        
    cuda_time = time.time() - start_t
    print(f"[Python GPU] Done. Computed and wrote {edge_count} unique edges in {cuda_time:.4f}s.")

if __name__ == "__main__":
    main()
