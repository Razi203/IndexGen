import sys
import os
import numpy as np
import subprocess
import tempfile
import time

def edit_distance(s1, s2):
    # Standard DP Levenshtein distance
    if len(s1) != len(s2):
        raise ValueError("Strings must have the same length")
    L = len(s1)
    # Python edit distance logic is simple if length is same and we only care about mismatches
    # But wait, edit distance allows insertions/deletions. We will implement simple DP.
    dp = np.zeros((L+1, L+1), dtype=np.int32)
    for i in range(L+1): dp[i, 0] = i
    for j in range(L+1): dp[0, j] = j
    for i in range(1, L+1):
        for j in range(1, L+1):
            if s1[i-1] == s2[j-1]:
                dp[i, j] = dp[i-1, j-1]
            else:
                dp[i, j] = 1 + min(dp[i-1, j], dp[i, j-1], dp[i-1, j-1])
    return dp[L, L]

def test_gpu_cluster():
    print("Generating random strings...")
    # Generate random strings of length 9
    L = 9
    N = 1000
    K = 10
    
    # Characters usually '0', '1', '2', '3'
    chars = ['0', '1', '2', '3']
    
    vectors = []
    for _ in range(N):
        vectors.append("".join(np.random.choice(chars, L)))
        
    centers = []
    for _ in range(K):
        centers.append("".join(np.random.choice(chars, L)))
        
    with tempfile.NamedTemporaryFile(mode='w', delete=False) as f_vec:
        vec_path = f_vec.name
        f_vec.write("\n".join(vectors))
        
    with tempfile.NamedTemporaryFile(mode='w', delete=False) as f_cen:
        cen_path = f_cen.name
        f_cen.write("\n".join(centers))
        
    out_path = vec_path + "_out.bin"
    
    # Run the script
    script_path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "gpu_cluster.py")
    cmd = ["python", script_path, vec_path, cen_path, out_path, "500"]
    
    print("Running GPU script...")
    start_t = time.time()
    subprocess.run(cmd, check=True)
    print(f"GPU Script took {time.time() - start_t:.4f}s")
    
    # Read assignments
    with open(out_path, "rb") as f:
        data = f.read()
    gpu_assignments = np.frombuffer(data, dtype=np.int32)
    
    print("Verifying assignments with CPU DP formulation...")
    errors = 0
    for i in range(N):
        vec = vectors[i]
        min_dist = 999
        min_c_idx = -1
        
        # Calculate exactly using standard Edit Distance
        for c_idx in range(K):
            d = edit_distance(vec, centers[c_idx])
            if d < min_dist:
                min_dist = d
                min_c_idx = c_idx
                
        # Compare
        gpu_assigned_c_idx = gpu_assignments[i]
        
        # The exact min center can be ambiguous if multiple centers have the same distance.
        # But the distance MUST be the minimal distance!
        gpu_c_dist = edit_distance(vec, centers[gpu_assigned_c_idx])
        
        if gpu_c_dist != min_dist:
            print(f"Mismatch at index {i}: GPU chose {gpu_assigned_c_idx} (dist {gpu_c_dist}), CPU min is {min_c_idx} (dist {min_dist})")
            errors += 1

    if errors == 0:
        print("Test Passed: All GPU assignments successfully correspond to minimum edit distance!")
    else:
        print(f"Test Failed: {errors} vectors had non-minimum distance assignments.")
        
    os.remove(vec_path)
    os.remove(cen_path)
    os.remove(out_path)

if __name__ == "__main__":
    test_gpu_cluster()
