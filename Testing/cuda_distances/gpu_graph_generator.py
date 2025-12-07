import sys
import os
import numpy as np
import time
from cuda_edit_distance import solve_edit_distance_cuda

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
        print("Usage: python gpu_graph_generator.py <input_vectors.txt> <output_edges.bin> <threshold> [max_gpu_memory_gb]")
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
    print(f"[Python GPU] Loaded {N} strings. Running CUDA Solver with T={threshold}, Memory Limit={max_gpu_gb}GB...")

    # 2. Run Optimized CUDA Kernel
    # Returns sparse list of lists: adj[i] = [neighbor_1, neighbor_2...]
    start_t = time.time()
    try:
        adj_list = solve_edit_distance_cuda(strings, threshold, tile_size=16384, max_gpu_memory_gb=max_gpu_gb)
    except Exception as e:
        print(f"[Error] CUDA computation failed: {e}")
        sys.exit(1)
        
    cuda_time = time.time() - start_t
    print(f"[Python GPU] CUDA Compute finished in {cuda_time:.4f}s")

    # 3. Flatten and Write Binary
    # The C++ AdjList expects manual symmetry (Set(i,j) and Set(j,i)).
    # However, to save disk I/O, we only write unique pairs (i, j) where i < j.
    # The C++ loader will handle the symmetric insertion.
    
    print(f"[Python GPU] Writing edges to binary file {output_file}...")
    start_t = time.time()
    edge_count = 0
    with open(output_file, 'wb') as f_out:
        buffer = []
        for i, neighbors in enumerate(adj_list):
            for neighbor in neighbors:
                # We strictly only save i < neighbor to avoid duplicates in file
                if i < neighbor:
                    buffer.append(i)
                    buffer.append(neighbor)
                    
                    if len(buffer) >= WRITE_BUFFER_SIZE:
                        # Pack as C-style int32 array
                        data = np.array(buffer, dtype=np.int32)
                        f_out.write(data.tobytes())
                        edge_count += (len(buffer) // 2)
                        buffer = []
        
        # Write remaining
        if buffer:
            data = np.array(buffer, dtype=np.int32)
            f_out.write(data.tobytes())
            edge_count += (len(buffer) // 2)

    write_time = time.time() - start_t
    print(f"[Python GPU] Done. Wrote {edge_count} unique edges in {write_time:.4f}s.")

if __name__ == "__main__":
    main()