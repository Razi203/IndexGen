import sys
import os
import numpy as np
import time

# Add src to python path to import the scripts
sys.path.append(os.path.join(os.path.dirname(__file__), '../../src'))

# Import original and new
from cuda_edit_distance import solve_edit_distance_cuda as solve_ed_orig
from cuda_edit_distance_binary import solve_edit_distance_cuda_binary as solve_ed_new

from gpu_cluster import solve_assignments_cuda as solve_asn_orig
from gpu_cluster_binary import solve_assignments_cuda_binary as solve_asn_new

def test_edit_distance():
    print("=== Testing Edit Distance ===")
    strings = [
        "10101010",
        "10101011",
        "00101010",
        "11111111",
        "00000000",
        "10111010",
        "01010101",
        "10101010"
    ]
    
    threshold = 3
    print("Testing original ED...")
    adj_orig = solve_ed_orig(strings, threshold)
    print("Testing new binary ED...")
    adj_new = solve_ed_new(strings, threshold)
    
    # Sort for exact sub-array comparison matching
    adj_orig = [sorted(l) for l in adj_orig]
    adj_new = [sorted(l) for l in adj_new]
    
    print("Original ADJ:", adj_orig)
    print("New ADJ:     ", adj_new)
    
    assert adj_orig == adj_new, "Edit distance matrices mismatch!"
    print("-> Edit Distance matches identically!\n")

def test_gpu_cluster():
    print("=== Testing GPU Cluster ===")
    vectors = [
        "10101010", "11110000", "00001111", "10101011", "01010101", "11111111"
    ] * 10 
    centers = [
        "10101010", "11111111", "00000000"
    ]
    
    print("Testing original Cluster...")
    asn_orig = solve_asn_orig(vectors, centers)
    print("Testing new binary Cluster...")
    asn_new = solve_asn_new(vectors, centers)
    
    print("Original Assignments:", list(asn_orig[:10]))
    print("New Assignments:     ", list(asn_new[:10]))
    
    assert np.array_equal(asn_orig, asn_new), "Assignments mismatch!"
    print("-> Cluster assignments match identically!\n")

if __name__ == "__main__":
    test_edit_distance()
    test_gpu_cluster()
