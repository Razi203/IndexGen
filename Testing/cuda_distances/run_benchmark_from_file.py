import subprocess
import time
import os
import sys
import numpy as np
from cuda_edit_distance import solve_edit_distance_cuda

# -------------------------------------------------------------------------
# Configuration
# -------------------------------------------------------------------------
THRESHOLD = 3         # Distance threshold
DATA_FILE = "vectors14.txt"
CPP_EXE = "./cpp_runner"

def load_data(filename):
    print(f"Loading data from {filename}...")
    if not os.path.exists(filename):
        print(f"Error: {filename} not found.")
        sys.exit(1)
        
    with open(filename, "r") as f:
        strings = [line.strip() for line in f if line.strip()]
        
    if not strings:
        print("Error: No data found in file.")
        sys.exit(1)
        
    n_strings = len(strings)
    string_len = len(strings[0])
    
    # Verify all strings have the same length (optional but good for sanity)
    for s in strings:
        if len(s) != string_len:
            print(f"Warning: String length mismatch. Expected {string_len}, found {len(s)}")
            # We might want to handle this, but for now just warn
            
    print(f"Loaded {n_strings} strings of length {string_len}")
    return strings, n_strings, string_len

def compile_cpp():
    print("Compiling C++ implementation...")
    # Check for EditDistance.hpp
    if not os.path.exists("EditDistance.hpp"):
        print("Error: EditDistance.hpp not found in current directory.")
        sys.exit(1)
        
    cmd = ["g++", "-O3", "-fopenmp", "-o", "cpp_runner", "cpp_runner.cpp"]
    try:
        subprocess.check_call(cmd)
        print("Compilation successful.")
    except subprocess.CalledProcessError:
        print("Compilation failed. Ensure g++ is installed.")
        sys.exit(1)

def run_cpp():
    print("\n--- Running C++ (OpenMP + AVX implied) ---")
    try:
        # Pass DATA_FILE directly to C++ runner
        result = subprocess.run([CPP_EXE, DATA_FILE, str(THRESHOLD)], capture_output=True, text=True)
        print(result.stdout)
        
        # Parse output for comparison
        time_sec = 0.0
        edges = 0
        for line in result.stdout.splitlines():
            if "CPP_TIME:" in line:
                time_sec = float(line.split()[1])
            if "CPP_EDGES:" in line:
                edges = int(line.split()[1])
        return time_sec, edges
    except Exception as e:
        print(f"Error running C++: {e}")
        return None, None

def run_python_cuda(strings):
    print("\n--- Running Python + CUDA ---")
    print(" -> Passing loaded strings to CUDA solver...")

    start_time = time.time()
    
    # Run solver
    # Now returns a Sparse List of Lists: [[neighbors of 0], [neighbors of 1], ...]
    adj_list = solve_edit_distance_cuda(strings, THRESHOLD, 16384)
    
    end_time = time.time()
    elapsed = end_time - start_time
    
    # Calculate unique edges. 
    # adj_list contains symmetric entries (i->j and j->i). 
    # Total edge pairs = sum(lengths) / 2
    output_filename = "cuda_adj_list.txt"
    print(f" -> Writing adjacency list to {output_filename}...")
    with open(output_filename, "w") as f:
        for i, neighbors in enumerate(adj_list):
            f.write(f"{i}: {','.join(map(str, neighbors))}\n")
    print(f" -> Adjacency list written to {output_filename}")

    total_entries = sum(len(neighbors) for neighbors in adj_list)
    edges = total_entries // 2
    
    print(f"CUDA Total Time (incl. transfer): {elapsed:.4f} seconds")
    print(f"CUDA Edges found: {edges}")
    
    return elapsed, edges

def main():
    # 1. Load Data
    strings, n_strings, string_len = load_data(DATA_FILE)
    
    # 2. Compile C++
    # compile_cpp()
    
    # 3. Run Both
    cuda_time, cuda_edges = run_python_cuda(strings)
    # cpp_time, cpp_edges = run_cpp()
    
    # 4. Compare
    print("\n--- Summary ---")
    print(f"Data: {n_strings} vectors of length {string_len}")
    # if cpp_time is not None:
    #     print(f"C++ Time:  {cpp_time:.4f}s")
    print(f"CUDA Time: {cuda_time:.4f}s")
    
    # if cpp_time is not None:
    #     if cuda_time < cpp_time:
    #         speedup = cpp_time / cuda_time
    #         print(f"Result: CUDA is {speedup:.2f}x faster.")
    #     else:
    #         speedup = cuda_time / cpp_time
    #         print(f"Result: C++ is {speedup:.2f}x faster.")
            
    #     if cuda_edges == cpp_edges:
    #         print("Verification: SUCCESS (Edge counts match)")
    #     else:
    #         print(f"Verification: FAILED (CUDA {cuda_edges} vs C++ {cpp_edges})")

if __name__ == "__main__":
    main()
