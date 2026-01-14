import os
import sys
import subprocess
import json
import numpy as np

def run_command(cmd):
    print(f"Running: {cmd}")
    ret = os.system(cmd)
    if ret != 0:
        print(f"Error executing command: {cmd}")
        sys.exit(1)

def main():
    if len(sys.argv) < 2:
        print("Usage: python detect_consistency.py <config.json>")
        sys.exit(1)

    config_path = sys.argv[1]
    
    # 0. Compile the C++ tool
    # We need to link against objects in build/
    print("Compiling C++ verification tool...")
    
    # Identify objects. We need to exclude IndexGen.o and testing.o since they have main()
    # But we need CandidateGenerator.o, Utils.o, etc.
    # We can rely on `make` having run, or just compile expected sources.
    # Assuming `make` has run and objects are in `build/`.
    
    # If not, let's run make first just in case.
    run_command("make -j4")
    
    # List of object files to link
    # We need to find them.
    # Based on Makefile:
    # TEST_SOURCES = $(SRC_DIR)/testing.cpp ...
    # We can just manually list the necessary ones from src/
    
    src_dir = "src"
    build_dir = "build"
    
    # Core objects needed
    objs = [
        "Candidates/LinearCodes.o",
        "Candidates/GF4.o", 
        "Candidates/GenMat.o",
        "Candidates/VTCodes.o",
        "Candidates/DifferentialVTCodes.o",
        "Utils.o",
        "CandidateGenerator.o",
        # SparseMat.o is NOT needed since we implemented edge logic in the C++ file directly using header
    ]
    
    obj_paths = [os.path.join(build_dir, o) for o in objs]
    
    # Check if they exist
    for o in obj_paths:
        if not os.path.exists(o):
            print(f"Missing object file: {o}")
            print("Please run `make` first or ensure build directory is populated.")
            sys.exit(1)
            
    cpp_src = "tests/dump_candidates_and_cpu_edges.cpp"
    exe_name = "tests/dump_tool"
    
    compile_cmd = f"g++ -std=c++17 -O3 -pthread -Iinclude {cpp_src} {' '.join(obj_paths)} -o {exe_name}"
    run_command(compile_cmd)
    
    # 1. Run C++ Tool -> candidates.txt, cpu_edges.bin
    print("\n[1/3] Running C++ Tool (Candidates + CPU Edges)...")
    run_command(f"{exe_name} {config_path}")
    
    # 2. Run GPU Script -> gpu_edges.bin
    print("\n[2/3] Running GPU Script...")
    
    # We need threshold from config
    with open(config_path) as f:
        config = json.load(f)
    
    # default editDist
    threshold = 4
    if "core" in config:
        threshold = config["core"].get("editDist", 4) # This is usually MinED. Algorithm uses < MinED (so dist <= MinED-1).
        # Wait, gpu_graph_generator.py takes threshold.
        # SparseMat.cpp: FillAdjListGPU passes `minED`.
        # solve_edit_distance_cuda: "Returns sparse list ... where dist < threshold".
        # In SparseMat.cpp: 
        #   FillAdjListGPU(..., const int minED, ...)
        #   cmd = "... " + to_string(minED) + " ..."
        # So it passes minED directly.
    
    # Also need maxGPUMemory
    max_gpu = 10.0
    if "performance" in config:
        max_gpu = config["performance"].get("max_gpu_memory_gb", 10.0)

    # Note: gpu_graph_generator.py usage: <input_vectors.txt> <output_edges.bin> <threshold> [max_gpu_memory_gb]
    # IMPORTANT: The GPU script finds edges where dist < threshold.
    # The C++ code finds edges where dist < minED. 
    # Example: if minED = 3, we want edges with dist 0, 1, 2.
    # So assuming `threshold` argument to GPU script is exactly `minED`, they match.
    
    cand_file = "tests/candidates.txt"
    gpu_edges_file = "tests/gpu_edges.bin"
    
    gpu_cmd = f"{sys.executable} scripts/gpu_graph_generator.py {cand_file} {gpu_edges_file} {threshold} {max_gpu}"
    run_command(gpu_cmd)
    
    # 3. Compare Edges
    print("\n[3/3] Comparing Results...")
    
    cpu_edges_file = "tests/cpu_edges.bin"
    
    def load_edges(filename):
        if not os.path.exists(filename):
             return set()
        data = np.fromfile(filename, dtype=np.int32)
        # Reshape to Nx2
        if len(data) == 0:
            return set()
        edges = data.reshape(-1, 2)
        # Ensure i < j
        # The tools should already output i < j but let's enforce it for safety in comparison
        # Actually standard set of tuples
        s = set()
        for i, j in edges:
            if i < j:
                s.add((i, j))
            else:
                s.add((j, i))
        return s

    cpu_edges = load_edges(cpu_edges_file)
    gpu_edges = load_edges(gpu_edges_file)
    
    print(f"CPU Edge Count: {len(cpu_edges)}")
    print(f"GPU Edge Count: {len(gpu_edges)}")
    
    if len(cpu_edges) != len(gpu_edges):
        print("Different output (count mismatch)")
        # Show differences
        diff_cpu_gpu = cpu_edges - gpu_edges
        diff_gpu_cpu = gpu_edges - cpu_edges
        if diff_cpu_gpu:
            print(f"Edges in CPU but not GPU ({len(diff_cpu_gpu)}): Sample: {list(diff_cpu_gpu)[:5]}...")
        if diff_gpu_cpu:
            print(f"Edges in GPU but not CPU ({len(diff_gpu_cpu)}): Sample: {list(diff_gpu_cpu)[:5]}...")
        sys.exit(1)
    
    import hashlib
    def get_edges_checksum(edges_set):
        # Sort edges to ensure deterministic string representation
        sorted_edges = sorted(list(edges_set))
        # Create a string representation
        data_str = str(sorted_edges).encode('utf-8')
        return hashlib.md5(data_str).hexdigest()

    print("Computing checksums for edge sets...")
    cpu_checksum = get_edges_checksum(cpu_edges)
    gpu_checksum = get_edges_checksum(gpu_edges)
    
    print(f"CPU Edges Checksum: {cpu_checksum}")
    print(f"GPU Edges Checksum: {gpu_checksum}")

    if cpu_checksum != gpu_checksum:
        print("Checksum mismatch! Content is different.")

    # If counts match, check content
    if cpu_edges == gpu_edges:
        print("Identical outputs (Verified by Set Equality and Checksum)")
    else:
        if sorted(list(cpu_edges)) == sorted(list(gpu_edges)):
            print("Identical outputs, different order")
        # Different content but same count? Rare but possible
        print("Different output (content mismatch)")
        diff_cpu_gpu = cpu_edges - gpu_edges
        diff_gpu_cpu = gpu_edges - cpu_edges
        print(f"Edges in CPU but not GPU: {len(diff_cpu_gpu)}")
        print(f"Edges in GPU but not CPU: {len(diff_gpu_cpu)}")
        sys.exit(1)

if __name__ == "__main__":
    main()
