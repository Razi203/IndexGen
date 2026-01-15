# IndexGen — DNA Codebook Generator

<p align="center">
  <strong>A High-Performance, GPU-Accelerated Tool for Generating Error-Correcting DNA Codebooks</strong>
</p>

---

**IndexGen** is a flexible, research-grade tool for generating DNA-based codebooks. It produces sets of DNA sequences (codewords) that maintain a guaranteed minimum Levenshtein (edit) distance between any two members, making them highly robust against insertions, deletions, and substitution errors—a critical requirement for applications in **DNA storage**, **molecular barcoding**, and **synthetic biology**.

**NOTE:** The script assumes the following mapping: A=0, C=1, G=2, T=3

---

## ✨ Key Features

-   **Multiple Generation Methods**: Choose from Linear Codes (GF(4)), Varshamov-Tenengolts (VT) Codes, Differential VT Codes, Random generation, or read candidates from a file.
-   **GPU Acceleration**: Leverage CUDA-enabled GPUs via Numba to accelerate the computationally intensive edit distance calculations by orders of magnitude.
-   **Biological Constraints**: Filter candidates by GC-content and maximum homopolymer run length for synthesizability.
-   **Resumable Jobs**: Long-running jobs save progress automatically and can be resumed from checkpoints.
-   **Cluster-Based Solving**: Optionally use K-Means clustering to partition candidates into clusters, solve each cluster independently, and iteratively refine until convergence.
-   **Extensible Architecture**: A clean Strategy Pattern design allows easy addition of new candidate generation methods and clustering algorithms.

---

## 📋 Table of Contents

1.  [Prerequisites](#prerequisites)
2.  [Installation](#installation)
3.  [Quick Start](#quick-start)
4.  [Usage](#usage)
5.  [Configuration](#configuration)
6.  [Repository Structure](#repository-structure)
7.  [How It Works](#how-it-works)
8.  [Extending IndexGen](#extending-indexgen)
9.  [Scripts Reference](#scripts-reference)

---

## Prerequisites

### Build Tools
-   A C++ compiler supporting **C++17** (e.g., `g++ >= 8`)
-   GNU `make`

### For GPU Acceleration (Optional, Recommended)
-   A CUDA-capable NVIDIA GPU
-   A Python environment with:
    -   `numba` (with CUDA support)
    -   `numpy`

A `conda` environment file (`env.yml`) is provided for easy setup:

```bash
conda env create -f env.yml
conda activate cuda_env
```

---

## Installation

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/your-repo/IndexGen.git
    cd IndexGen
    ```

2.  **Compile the project:**
    ```bash
    make
    ```
    This creates the `IndexGen` executable in the project root.

3.  **(Optional) Set up the Python GPU environment:**
    ```bash
    conda env create -f env.yml
    conda activate cuda_env
    ```

---

## Quick Start

The fastest way to run IndexGen is with a JSON configuration file.

**1. Create a configuration file** (or use an existing one from `config/`):

```json
{
    "dir": "my_output",
    "core": { "lenStart": 11, "lenEnd": 11, "editDist": 3 },
    "constraints": { "maxRun": 3, "minGC": 0.3, "maxGC": 0.7 },
    "performance": { "threads": 16, "use_gpu": true },
    "method": { "name": "LinearCode", "linearCode": { "minHD": 3 } }
}
```

**2. Run the generator:**

```bash
./IndexGen --config config/config.json
```

**3. Find your results** in the output directory (e.g., `my_output/`):
-   `Codebook_n11_d3.txt` — The final list of codewords.
-   `log.log` — A detailed log with timing statistics and parameters.

---

## Usage

IndexGen can be configured using either **command-line arguments** or a **JSON configuration file**. The configuration file is recommended for complex setups.

> **Note:** Command-line arguments take priority over configuration file values. You can use a config file for defaults and override specific parameters via CLI.

### Basic Commands

```bash
# Run with a config file (recommended)
./IndexGen --config path/to/config.json

# Run with command-line arguments
./IndexGen --lenStart 10 --lenEnd 12 --editDist 4 --method LinearCode --minHD 3

# Resume an interrupted job
./IndexGen --resume --dir 2024-01-01_12-00

# Get help on all available options
./IndexGen --help
```

### Command-Line Options Reference

| Option                | Short | Description                                                                 | Default      |
| :-------------------- | :---- | :-------------------------------------------------------------------------- | :----------- |
| `--help`              | `-h`  | Print usage information.                                                    |              |
| `--resume`            | `-r`  | Resume from a saved checkpoint in the specified `--dir`.                    | `false`      |
| `--config`            | `-c`  | Path to a JSON configuration file.                                          |              |
| `--dir`               | `-d`  | Output directory name. Auto-generates a timestamped name if empty.          | Auto         |
| `--lenStart`          | `-s`  | Starting codeword length.                                                   | `10`         |
| `--lenEnd`            | `-e`  | Ending codeword length (inclusive).                                         | `10`         |
| `--editDist`          | `-D`  | Minimum Levenshtein (edit) distance between codewords.                      | `4`          |
| `--maxRun`            |       | Maximum homopolymer run length (e.g., 3 forbids `AAAA`). 0 to disable.      | `3`          |
| `--minGC`             |       | Minimum GC-content fraction (0.0 to 1.0).                                   | `0.3`        |
| `--maxGC`             |       | Maximum GC-content fraction (0.0 to 1.0).                                   | `0.7`        |
| `--threads`           | `-t`  | Number of CPU threads.                                                      | `16`         |
| `--saveInterval`      |       | Checkpoint save interval in seconds.                                        | `80000`      |
| `--verify`            |       | Run post-generation verification of edit distances.                         | `false`      |
| `--gpu`               |       | Use GPU for edit distance computation.                                      | `true`       |
| `--maxGPUMemory`      |       | Maximum GPU memory usage in GB.                                             | `10.0`       |
| `--cluster`           |       | Enable K-Means clustering on the final codebook.                            | `false`      |
| `--numClusters`       |       | Target number of clusters (k).                                              | `500`        |
| `--clusterVerbose`    |       | Enable verbose timing/logging for clustering iterations.                    | `false`      |
| `--clusterConvergence`|       | Number of identical iterations required for convergence.                    | `3`          |
| `--method`            | `-m`  | Generation method. See [Generation Methods](#generation-methods).           | `LinearCode` |

---

## Configuration

For complex runs, a JSON configuration file is the best approach. See the **[CONFIG_GUIDE.md](CONFIG_GUIDE.md)** for a complete reference of all options.

Example configuration files are available in the `config/examples/` directory.

---

## Repository Structure

A quick overview of the project layout:

```
IndexGen/
├── IndexGen              # Main executable (after compilation)
├── Makefile              # Build configuration
├── README.md             # This file
├── CONFIG_GUIDE.md       # Detailed configuration reference
├── env.yml               # Conda environment for GPU support
│
├── config/               # Configuration files
│   ├── config.json       # Example GPU configuration
│   ├── config_CPU.json   # Example CPU-only configuration
│   └── examples/         # Annotated example configurations
│
├── include/              # C++ Header Files (.hpp)
│   ├── IndexGen.hpp      # Core data structures (Params, Constraints, Enums)
│   ├── Candidates.hpp    # Candidate generation interface
│   ├── CandidateGenerator.hpp # OOP Strategy Pattern for generators
│   ├── SparseMat.hpp     # Adjacency list (conflict graph) for codebook selection
│   ├── EditDistance.hpp  # Fast bit-parallel edit distance (Myers' algorithm)
│   ├── Utils.hpp         # Utility functions (I/O, distance metrics, filters)
│   ├── Decode.hpp        # Nearest-neighbor decoding logic
│   ├── MaxClique.hpp     # Alternative codebook selection via Max Clique
│   ├── Candidates/       # Headers for specific generation methods
│   │   ├── LinearCodes.hpp
│   │   ├── VTCodes.hpp
│   │   ├── DifferentialVTCodes.hpp
│   │   ├── FileRead.hpp
│   │   ├── GF4.hpp       # Galois Field GF(4) arithmetic
│   │   └── GenMat.hpp    # Generator matrix construction
│   └── clustering/       # Clustering interface
│       └── ClusteringInterface.hpp
│
├── src/                  # C++ Source Files (.cpp)
│   ├── IndexGen.cpp      # Main entry point (CLI parsing, orchestration)
│   ├── Candidates.cpp    # Candidate generation and filtering logic
│   ├── CandidateGenerator.cpp # Factory and generator implementations
│   ├── SparseMat.cpp     # Conflict graph building and greedy solver
│   ├── Utils.cpp         # Utility function implementations
│   ├── Decode.cpp        # Decoding implementation
│   ├── MaxClique.cpp     # Max Clique algorithm implementation
│   ├── Candidates/       # Implementations for generation methods
│   │   ├── LinearCodes.cpp
│   │   ├── VTCodes.cpp
│   │   ├── DifferentialVTCodes.cpp
│   │   ├── FileRead.cpp
│   │   ├── GF4.cpp
│   │   └── GenMat.cpp
│   ├── clustering/       # Clustering-based solving
│   │   ├── KMeansAdapter.cpp  # Adapter for swapping implementations
│   │   ├── KMeansAdapter.hpp
│   │   └── impl/         # Clustering algorithm implementations
│   ├── gpu_graph_generator.py  # Python script for GPU-accelerated graph generation
│   └── cuda_edit_distance.py   # CUDA kernels for edit distance (Numba)
│
├── scripts/              # Automation and analysis scripts
│   ├── run.sh            # Main run script (activates conda, runs IndexGen)
│   ├── run_batch.sh      # Run multiple configurations
│   ├── run_repeat.py     # Run experiments with different random seeds
│   ├── analyze_codewords.py # Analyze log files and generate histograms
│   └── extract_results.py   # Extract results from multiple runs
│
├── build/                # Compiled object files (created by make)
├── Results/              # Default location for past run outputs
└── Tests/                # Test outputs and experimental data
```

---

## How It Works

The codebook generation process follows these stages:

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         1. CANDIDATE GENERATION                         │
│  Generate an initial, large set of DNA strings using a coding-theoretic │
│  method (e.g., Linear Codes, VT Codes) that guarantees a minimum        │
│  Hamming distance.                                                      │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                          2. CONSTRAINT FILTERING                        │
│  Apply biological constraints to remove infeasible sequences:           │
│  - Maximum homopolymer run (e.g., no more than 3 consecutive A's)       │
│  - GC-content range (e.g., between 30% and 70%)                         │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                     3. CONFLICT GRAPH CONSTRUCTION                      │
│  Build a graph where each candidate is a node. An edge connects two     │
│  nodes if their Levenshtein distance is LESS than the required minimum. │
│  ► This is the most computationally intensive step.                     │
│  ► GPU acceleration is applied here (if enabled).                       │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                       4. INDEPENDENT SET SOLVER                         │
│  Find a Maximum Independent Set (MIS) in the conflict graph. The MIS    │
│  represents the largest subset of candidates where no two are too       │
│  similar, forming the final codebook.                                   │
│  ► A greedy heuristic is used for efficiency.                           │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      5. OUTPUT & POST-PROCESSING                        │
│  - Save the codebook (`Codebook_n<len>_d<dist>.txt`)                    │
│  - Save detailed logs (`log.log`)                                       │
│  - Optionally verify all pairwise distances                             │
└─────────────────────────────────────────────────────────────────────────┘
```

### Alternative: Cluster-Based Solving

When clustering is enabled, the solver uses an **iterative cluster-based approach** instead of solving the entire conflict graph at once:

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     CLUSTER-BASED CODEBOOK SOLVING                      │
│                                                                         │
│  1. Partition candidates into K clusters using K-Means                  │
│  2. For each cluster, solve the Independent Set problem independently   │
│  3. Combine the per-cluster solutions into a unified codebook           │
│  4. Repeat steps 1-3 until codebook size converges                      │
│     (no change for multiple consecutive iterations)                     │
│                                                                         │
│  ► Reduces memory requirements for very large candidate sets            │
│  ► May find different (sometimes larger) codebooks than direct solving  │
└─────────────────────────────────────────────────────────────────────────┘
```

### Generation Methods

IndexGen supports these candidate generation strategies:

| Method          | Description                                                                                                  |
| :-------------- | :----------------------------------------------------------------------------------------------------------- |
| **LinearCode**  | Generates candidates from linear codes over GF(4). Guarantees minimum Hamming distance. Most efficient.      |
| **VTCode**      | Uses Varshamov-Tenengolts codes, known for single insertion/deletion correction.                             |
| **Diff_VTCode** | Differential VT Codes, based on the differential sequence of the codeword.                                   |
| **Random**      | Generates uniformly random DNA strings. Simple but produces fewer final codewords.                          |
| **AllStrings**  | Enumerates all 4^n possible strings. Only feasible for very short lengths (n ≤ 10).                          |
| **FileRead**    | Reads candidates from an external text file. Useful for custom candidate sets.                              |

---

## Extending IndexGen

The application uses a **Strategy Pattern**, making it easy to add new candidate generation methods. See the detailed guide in the [legacy README](#extending-with-new-methods-legacy) or follow these high-level steps:

1.  **Define a new identifier** in `GenerationMethod` enum (`include/IndexGen.hpp`).
2.  **Create a Constraints struct** inheriting from `GenerationConstraints`.
3.  **Implement a Generator class** inheriting from `CandidateGenerator` (`include/CandidateGenerator.hpp`).
4.  **Update the factory function** `CreateGenerator()` in `src/CandidateGenerator.cpp`.
5.  **Add JSON parsing** in `LoadParamsFromJson()` (`src/Utils.cpp`).
6.  **Add CLI options** in `configure_parser()` (`src/IndexGen.cpp`).

---

## Replacing the Clustering Implementation

The clustering subsystem uses an **Adapter Pattern** to allow easy replacement of the underlying clustering algorithm. The current implementation is a basic hierarchical K-Means, but you can swap it for any algorithm that implements the `IClustering` interface.

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         CLUSTERING ARCHITECTURE                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  include/clustering/ClusteringInterface.hpp                             │
│  └── IClustering (abstract interface)                                   │
│       └── virtual cluster(data) = 0                                     │
│                                                                         │
│  src/clustering/KMeansAdapter.cpp/.hpp                                  │
│  └── KMeansAdapter : IClustering                                        │
│       └── Wraps the current implementation                              │
│       └── Easy to swap to a different impl                              │
│                                                                         │
│  src/clustering/impl/                                                   │
│  └── hierarchical_kmeans/   (current implementation)                    │
│  └── place_holder/          (for future implementations)                │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### How to Replace the Implementation

1.  **Create your new implementation** in `src/clustering/impl/your_algorithm/`:
    ```cpp
    // Example: src/clustering/impl/your_algorithm/YourClustering.hpp
    class YourClustering {
    public:
        void fit(const std::vector<std::string>& data);
        std::vector<std::vector<std::string>> getClusters();
    };
    ```

2.  **Update `KMeansAdapter`** to use your implementation:
    ```cpp
    // In src/clustering/KMeansAdapter.cpp
    #include "impl/your_algorithm/YourClustering.hpp"
    
    std::vector<std::vector<std::string>> KMeansAdapter::cluster(
        const std::vector<std::string>& data) {
        YourClustering algo;
        algo.fit(data);
        return algo.getClusters();
    }
    ```

3.  **Or create a new adapter** implementing `IClustering`:
    ```cpp
    class YourAdapter : public IClustering {
        std::vector<std::vector<std::string>> cluster(
            const std::vector<std::string>& data) override;
    };
    ```

### Why Use an Adapter?

-   **Decoupling**: The main codebook generation logic doesn't depend on a specific clustering implementation.
-   **Easy Experimentation**: Swap implementations without modifying core code.
-   **Future-Proofing**: New algorithms can be added to `impl/` and switched by editing only the adapter.

> **Note**: The current hierarchical K-Means implementation is a rough/basic version. The adapter pattern was specifically designed to facilitate replacing it with more sophisticated algorithms in the future.

---

## Scripts Reference

Utility scripts for running and analyzing experiments:

| Script                  | Description                                                                        |
| :---------------------- | :--------------------------------------------------------------------------------- |
| `scripts/run.sh`        | Activates conda environment and runs IndexGen with a config file.                  |
| `scripts/run_batch.sh`  | Runs multiple configurations sequentially, useful for parameter sweeps.            |
| `scripts/run_repeat.py` | Runs the same configuration multiple times with different random seeds.            |
| `scripts/analyze_codewords.py` | Parses log files and generates histogram visualizations of results.          |
| `scripts/extract_results.py`   | Extracts key metrics from multiple run directories into a summary.            |

---

## Extending with New Methods (Legacy)

<details>
<summary>Click to expand the detailed guide for adding new generation methods</summary>

The application uses a **Strategy Pattern**, making it easy to add new candidate generation methods. To add a new method called `NEW_METHOD`, follow these steps.

### Step 1: Define the Method Identifier

Add a unique identifier for your new method to the `GenerationMethod` enum.

* **File**: `IndexGen.hpp`
* **Action**: Add `NEW_METHOD` to the `enum class`.

```cpp
enum class GenerationMethod
{
    LINEAR_CODE,
    RANDOM,
    ALL_STRINGS,
    .
    .
    .
    NEW_METHOD // <-- Add your new method here
};
```

### Step 2: Create a New Constraints Struct

Create a `struct` to hold any parameters specific to your new method. This struct must inherit from `GenerationConstraints`.

* **File**: `IndexGen.hpp`
* **Action**: Define `NewMethodConstraints`.

```cpp
struct NewMethodConstraints : public GenerationConstraints
{
    // Add any parameters your method needs
    int new_parameter_1;
    double new_parameter_2;

    // Create a constructor for easy initialization
    NewMethodConstraints(int p1, double p2)
        : GenerationConstraints(), new_parameter_1(p1), new_parameter_2(p2) {}
};
```

### Step 3: Implement the Generation Logic

Add a new `case` to the `switch` statement in the `Candidates` function to call your generation logic.

* **File**: `Candidates.cpp`
* **Action**: Add a `case` for `NEW_METHOD` inside the `Candidates` function.

```cpp
// In the Candidates function...
switch (params.method)
{
    // ... (existing cases)

    case GenerationMethod::NEW_METHOD: {
        auto* constraints = dynamic_cast<NewMethodConstraints*>(params.constraints.get());
        if (!constraints) {
            throw std::runtime_error("Invalid constraints for NEW_METHOD.");
        }
        // Call your new generation function with its specific parameters
        unfiltered = GenNewMethodStrings(params.codeLen, constraints->new_parameter_1, constraints->new_parameter_2);
        break;
    }
}
```

### Step 4: Update Utility Functions (I/O)

To ensure your new method's parameters can be **saved, loaded, and printed** correctly, you must update the I/O functions in `Utils.cpp`.

* **File**: `Utils.cpp`
* **Action**: Add a `case` for `NEW_METHOD` to all relevant `switch` statements.

#### 4.1. Serialization (Saving and Loading)

Update `ParamsToFile` to write your new parameters to the progress file and `FileToParams` to read them back.

* In `ParamsToFile`:

    ```cpp
    // ... inside the switch statement
    case GenerationMethod::NEW_METHOD: {
        auto* constraints = dynamic_cast<NewMethodConstraints*>(params.constraints.get());
        if (constraints) {
            output_file << constraints->new_parameter_1 << '\n';
            output_file << constraints->new_parameter_2 << '\n';
        }
        break;
    }
    ```

* In `FileToParams`:

    ```cpp
    // ... inside the switch statement
    case GenerationMethod::NEW_METHOD: {
        int p1;
        double p2;
        input_file >> p1;
        input_file >> p2;
        params.constraints = std::make_unique<NewMethodConstraints>(p1, p2);
        break;
    }
    ```

#### 4.2. Printing and Logging

Update the helper functions that print parameters to the console or log files. This is crucial for debugging and keeping records.

* In `GenerationMethodToString`:

    ```cpp
    // ... inside the switch statement
    case GenerationMethod::NEW_METHOD:
        return "New Method Name";
    ```

* In `PrintParamsToFile` and `PrintTestParams`:

    ```cpp
    // ... inside the switch statement
    case GenerationMethod::NEW_METHOD: {
        auto *constraints = dynamic_cast<NewMethodConstraints *>(params.constraints.get());
        if (constraints) {
            output_file << "New Parameter 1:\t\t" << constraints->new_parameter_1 << endl;
            output_file << "New Parameter 2:\t\t" << constraints->new_parameter_2 << endl;
        }
        break;
    }
    ```

### Step 5: Add Command-Line Options

Finally, make your new method configurable from the command line.

* **File**: `IndexGen.cpp`
* **Action**: Update the `configure_parser` and `main` functions.

1. **Add the options** in `configure_parser`:

    ```cpp
    void configure_parser(cxxopts::Options &options)
    {
        options.add_options()
            // ... (existing options)
            ("new_param_1", "Description for new parameter 1", cxxopts::value<int>()->default_value("0"))
            ("new_param_2", "Description for new parameter 2", cxxopts::value<double>()->default_value("0.0"));
    }
    ```

2. **Handle the new method** in `main`. Add an `else if` block to parse the new parameters and create your custom constraints object.

    ```cpp
    // In main()...
    string method_str = result["method"].as<string>();

    if (method_str == "LinearCode") {
        // ...
    }
    // ... (other methods)
    else if (method_str == "NEW_METHOD")
    {
        params.method = GenerationMethod::NEW_METHOD;
        int p1 = result["new_param_1"].as<int>();
        double p2 = result["new_param_2"].as<double>();
        params.constraints = make_unique<NewMethodConstraints>(p1, p2);
        cout << "Using Generation Method: NEW_METHOD (p1=" << p1 << ", p2=" << p2 << ")" << endl;
    }
    else
    {
        cerr << "Error: Unknown generation method '" << method_str << "'." << endl;
        return 1;
    }
    ```

</details>
