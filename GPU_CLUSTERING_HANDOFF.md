# Handoff Doc: GPU-Accelerated Random Clustering

## Objective
Implement a clustering method that selects $K$ random vectors as centers and assigns all other vectors to the nearest center based on exact edit distance, leveraging GPU acceleration.

## Implemented Components

### 1. GPU Script (`src/gpu_cluster.py`)
- **Kernel**: Uses Peaucellier-Myers bit-parallel algorithm for fast edit distance.
- **Optimization**: Handles pairwise distances in batches ($K \times BATCH\_SIZE$) to keep VRAM usage under control ($O(K)$ per batch).
- **Output**: Binary file of 32-bit integers (`assignments.bin`) representing the index of the closest center for each vector.

### 2. C++ RandomCluster (`src/clustering/impl/random_cluster/`)
- **Role**: Manages center selection, file I/O for Python interaction, and cluster grouping.
- **Integration**: Plumbed into `KMeansAdapter` and controlled via config.

### 3. Configuration System
- Added `method` field to `ClusteringParams` in `include/IndexGen.hpp`.
- Updated `src/Utils.cpp` to parse this from `config.json`.
- Updated `config/config.json` to use `"method": "random_cluster"`.

## Verification Steps for Next Agent

### Environment Requirement
- Linux with NVIDIA GPU and CUDA.
- Python 3 with `cupy` and `numpy` installed.
- Conda environment `cuda_env` is recommended.

### Automated Test
1.  Navigate to `Tests/`.
2.  Run `python3 test_gpu_cluster.py`. (The script might need adjusting, because it was moved manually w\o corrections)
3.  **Expected Output**: `SUCCESS: All assignments verified against CPU implementation!`.
    - This test generates random strings (length 9), runs the GPU script, and cross-references results with a slow but correct CPU-based edit distance function.

### Running in IndexGen
To run the full simulation with GPU clustering:
1.  Ensure `config/config.json` has:
    ```json
    "clustering": {
        "enabled": true,
        "method": "random_cluster",
        "k": 50,
        ...
    }
    ```
2.  Compile and run `./IndexGen`.
3.  Monitor output for `[C++ RandomCluster] Executing GPU Cluster: python ...`.

## Known Constraints
- Sequence length for `gpu_cluster.py`'s bit-parallel kernel is optimized for words that fit in a 64-bit integer (len $\le 64$).
- The current implementation uses `system()` calls to `python`. Ensure `python` or `python3` is in path (script uses `python3` in the CLI call).

---
*Created by Antigravity (Advanced Agentic Coding)*
