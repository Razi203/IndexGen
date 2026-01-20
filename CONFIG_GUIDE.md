# IndexGen Configuration Guide

This document provides a complete reference for all configuration options in IndexGen. Configuration can be provided via a JSON file (recommended) or command-line arguments.

---

## Table of Contents

1.  [Configuration Overview](#configuration-overview)
2.  [Global Settings](#global-settings)
3.  [Core Settings](#core-settings)
4.  [Constraints](#constraints)
5.  [Performance Settings](#performance-settings)
6.  [Clustering Settings](#clustering-settings)
7.  [Generation Methods](#generation-methods)
    -   [LinearCode](#1-linearcode-method)
    -   [VTCode](#2-vtcode-method)
    -   [Diff_VTCode](#3-differential-vtcode-method)
    -   [Random](#4-random-method)
    -   [AllStrings](#5-allstrings-method)
    -   [FileRead](#6-fileread-method)
8.  [Complete Example](#complete-example)
9.  [Example Configurations](#example-configurations)

---

## Configuration Overview

IndexGen uses a hierarchical JSON format for configuration. The structure is:

```json
{
    "dir": "...",
    "verify": false,
    "core": { ... },
    "constraints": { ... },
    "performance": { ... },
    "clustering": { ... },
    "method": { ... }
}
```

### Loading a Configuration

```bash
./IndexGen --config path/to/config.json
```

Any parameter omitted from the JSON file will use its default value. **Command-line arguments allow you to override specific values from the configuration file.** For example, you can load a standard config but change the convergence threshold just for one run:

```bash
./IndexGen --config standard_config.json --clusterConvergence 5
```

---

## Global Settings

These settings control general behavior that doesn't fit into other categories.

| Key      | Type      | Default | CLI Flag     | Description                                                                                                |
| :------- | :-------- | :------ | :----------- | :--------------------------------------------------------------------------------------------------------- |
| `dir`    | `string`  | Auto    | `--dir`      | Output directory name. If empty or omitted, a timestamped directory is created (e.g., `2024-01-15_10-30`). Nested paths like `"Results/Run1"` are supported. |
| `verify` | `boolean` | `false` | `--verify`   | If `true`, runs a verification step after generation to confirm all codeword pairs meet the minimum edit distance. Useful for validation but adds computation time. |

### Example
```json
{
    "dir": "output/experiment_001",
    "verify": true
}
```

---

## Core Settings

The `core` object defines the fundamental properties of the codebook to be generated.

| Key        | Type      | Default | CLI Flag       | Description                                                                                     |
| :--------- | :-------- | :------ | :------------- | :---------------------------------------------------------------------------------------------- |
| `lenStart` | `integer` | `10`    | `--lenStart`   | The starting codeword length for the generation loop. The generator iterates from `lenStart` to `lenEnd`. |
| `lenEnd`   | `integer` | `10`    | `--lenEnd`     | The ending codeword length (inclusive). Set equal to `lenStart` to generate for a single length. |
| `editDist` | `integer` | `4`     | `--editDist`   | The **minimum Levenshtein (edit) distance** required between any two codewords in the final codebook. Common values: 3, 4, or 5. Higher values yield fewer but more robust codewords. |

### Example
```json
{
    "core": {
        "lenStart": 12,
        "lenEnd": 15,
        "editDist": 4
    }
}
```
This generates codebooks for lengths 12, 13, 14, and 15, each with minimum edit distance 4.

---

## Constraints

The `constraints` object defines biological/physical constraints applied to filter candidate codewords.

| Key      | Type    | Default | CLI Flag   | Description                                                                                                                                            |
| :------- | :------ | :------ | :--------- | :----------------------------------------------------------------------------------------------------------------------------------------------------- |
| `maxRun` | `integer` | `3`   | `--maxRun` | Maximum length of a **homopolymer run** (consecutive identical bases). A value of `3` forbids runs like `AAAA`. Set to `0` to **disable** this filter (allow any run length). |
| `minGC`  | `float` | `0.3`   | `--minGC`  | Minimum **GC-content** fraction (0.0 to 1.0). GC-content is the proportion of G and C bases in the sequence. A value of `0.3` requires at least 30% G+C. Set to `0` to disable. |
| `maxGC`  | `float` | `0.7`   | `--maxGC`  | Maximum **GC-content** fraction (0.0 to 1.0). A value of `0.7` requires at most 70% G+C. Set to `0` (or `1.0`) to disable.                             |

### Special Values
-   Setting `maxRun = 0` disables the homopolymer run filter entirely.
-   Setting `minGC = 0` and `maxGC = 0` (or `maxGC = 1.0`) disables GC-content filtering entirely.

### Example
```json
{
    "constraints": {
        "maxRun": 4,
        "minGC": 0.4,
        "maxGC": 0.6
    }
}
```
This allows runs of up to 4 identical bases and requires GC-content between 40% and 60%.

---

## Performance Settings

The `performance` object controls computational resources and execution behavior.

| Key                | Type      | Default  | CLI Flag           | Description                                                                                                                                               |
| :----------------- | :-------- | :------- | :----------------- | :-------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `threads`          | `integer` | `16`     | `--threads`        | Number of **CPU threads** for parallel tasks (candidate generation, filtering, CPU edit distance).                                                        |
| `saveInterval`     | `integer` | `80000`  | `--saveInterval`   | Interval in **seconds** between checkpoint saves. Progress is saved to allow resuming interrupted jobs. Smaller values = more frequent saves but more I/O. |
| `use_gpu`          | `boolean` | `true`   | `--gpu`            | If `true`, uses **GPU-accelerated** edit distance computation via Python/CUDA (requires the `cuda_env` conda environment). If `false`, uses multi-threaded CPU. |
| `max_gpu_memory_gb` | `float`  | `10.0`   | `--maxGPUMemory`   | Maximum **GPU memory** in GB to allocate. Adjusts batch sizes to stay within limits. Relevant only when `use_gpu = true`.                                  |

### GPU vs CPU
-   **GPU mode** (`use_gpu: true`): Orders of magnitude faster for large candidate sets (10,000+). Requires CUDA-capable NVIDIA GPU and Python environment.
-   **CPU mode** (`use_gpu: false`): No external dependencies. Suitable for smaller candidate sets or systems without GPUs.

### Example
```json
{
    "performance": {
        "threads": 32,
        "saveInterval": 3600,
        "use_gpu": true,
        "max_gpu_memory_gb": 8.0
    }
}
```

---

## Clustering Settings

The `clustering` object enables an **alternative cluster-based solving approach**. When enabled, instead of solving the entire conflict graph at once, the solver partitions candidates into clusters, solves each cluster independently, combines results, and iterates until the codebook size converges.

| Key       | Type      | Default | CLI Flag           | Description                                                                               |
| :-------- | :-------- | :------ | :----------------- | :---------------------------------------------------------------------------------------- |
| `enabled` | `boolean` | `false` | `--cluster`        | If `true`, uses cluster-based iterative solving instead of direct graph solving.          |
| `k`       | `integer` | `500`   | `--numClusters`    | Target number of clusters to partition candidates into.                                   |
| `verbose` | `boolean` | `false` | `--clusterVerbose` | If `true`, prints detailed timing and iteration information during clustering.            |
| `convergenceIterations` | `integer` | `3` | `--clusterConvergence` | Number of consecutive iterations with identical codebook size required to consider the process converged. |

### How Cluster-Based Solving Works

1. **Partition**: Divide the candidate set into `k` clusters using K-Means
2. **Solve**: Run the Independent Set solver on each cluster separately
3. **Combine**: Merge per-cluster solutions into a unified codebook
4. **Iterate**: Repeat steps 1-3 until the combined codebook size stops changing

### When to Use Clustering
-   **Very large candidate sets**: Reduces memory requirements by solving smaller subproblems
-   **Experimentation**: May find different (sometimes larger) codebooks than direct solving
-   **Memory-constrained systems**: Each cluster requires less memory than the full graph

### Example
```json
{
    "clustering": {
        "enabled": true,
        "k": 100,
        "verbose": true
    }
}
```

---

## Generation Methods

The `method` object specifies the algorithm for generating candidate codewords. The `name` field selects the method, and a nested object provides method-specific parameters.

### Available Methods

| Method Name   | Description                                                                                     |
| :------------ | :---------------------------------------------------------------------------------------------- |
| `LinearCode`  | Linear codes over GF(4) with guaranteed Hamming distance. Most efficient for large codebooks.   |
| `VTCode`      | Varshamov-Tenengolts codes for single insertion/deletion correction.                            |
| `Diff_VTCode` | Differential VT codes based on differential sequence properties.                                |
| `Random`      | Uniformly random DNA strings.                                                                   |
| `AllStrings`  | Enumerate all 4^n possible strings (only for small n).                                          |
| `FileRead`    | Read candidates from an external file.                                                          |

---

### 1. LinearCode Method

Generates candidates using **linear codes over GF(4)**. This is the most efficient method for producing large, high-quality codebooks. The generator constructs codewords with a guaranteed minimum Hamming distance, which correlates with edit distance for DNA.

```json
{
    "method": {
        "name": "LinearCode",
        "linearCode": {
            "minHD": 3,
            "biasMode": "default",
            "rowPermMode": "identity",
            "colPermMode": "identity",
            "randomSeed": 0,
            "bias": [],
            "rowPerm": [],
            "colPerm": []
        }
    }
}
```

#### Parameters

| Key           | Type        | Default      | Description                                                                                                                                     |
| :------------ | :---------- | :----------- | :---------------------------------------------------------------------------------------------------------------------------------------------- |
| `minHD`       | `integer`   | `3`          | Minimum **Hamming distance** for the linear code. Valid values: `2`, `3`, `4`, `5`. Higher values generate fewer candidates but with stronger guarantees. |
| `biasMode`    | `string`    | `"default"`  | How the bias vector is initialized. See [Vector Modes](#vector-modes).                                                                          |
| `rowPermMode` | `string`    | `"identity"` | How the row permutation is initialized. See [Vector Modes](#vector-modes).                                                                       |
| `colPermMode` | `string`    | `"identity"` | How the column permutation is initialized. See [Vector Modes](#vector-modes).                                                                    |
| `randomSeed`  | `integer`   | `0`          | Seed for random number generation (when using `"random"` modes). `0` uses current time.                                                         |
| `bias`        | `array[int]`| `[]`         | Manual bias vector (GF(4) values: 0-3). Required if `biasMode = "manual"`. Length must equal codeword length.                                    |
| `rowPerm`     | `array[int]`| `[]`         | Manual row permutation (0-indexed). Required if `rowPermMode = "manual"`. Length must equal number of rows in generator matrix.                  |
| `colPerm`     | `array[int]`| `[]`         | Manual column permutation (0-indexed). Required if `colPermMode = "manual"`. Length must equal codeword length.                                  |

#### Vector Modes

| Mode        | Description                                                                                             |
| :---------- | :------------------------------------------------------------------------------------------------------ |
| `"default"` | For bias: all zeros. For permutations: identity (0, 1, 2, ..., n-1).                                    |
| `"random"`  | Randomly generated using the `randomSeed`.                                                              |
| `"manual"`  | User-specified via the corresponding array (`bias`, `rowPerm`, or `colPerm`).                           |

#### About Bias and Permutations
-   **Bias vector**: Added to each codeword after encoding. Shifts the codebook in GF(4) space.
-   **Row permutation**: Reorders the rows of the generator matrix before encoding.
-   **Column permutation**: Reorders the symbols in the final codeword.

Experimenting with different biases and permutations can yield different final codebook sizes due to varying interactions with constraints and the conflict graph structure.

#### Example: Random Bias and Permutations
```json
{
    "method": {
        "name": "LinearCode",
        "linearCode": {
            "minHD": 3,
            "biasMode": "random",
            "rowPermMode": "random",
            "colPermMode": "random",
            "randomSeed": 42
        }
    }
}
```

---

### 2. VTCode Method

Generates candidates using **Varshamov-Tenengolts (VT) codes**. VT codes are designed for single insertion/deletion error correction and have nice combinatorial properties.

```json
{
    "method": {
        "name": "VTCode",
        "vtCode": {
            "a": 0,
            "b": 0
        }
    }
}
```

#### Parameters

| Key | Type      | Default | Description                                                                                                                          |
| :-- | :-------- | :------ | :----------------------------------------------------------------------------------------------------------------------------------- |
| `a` | `integer` | `0`     | Remainder parameter for the positional sum condition: `sum_{i=2}^{n} (i-1) * alpha_i ≡ a (mod n)`. Valid range: `0` to `n-1`.       |
| `b` | `integer` | `0`     | Remainder parameter for the symbol sum condition: `sum_{j=1}^{n} x_j ≡ b (mod 4)`. Valid range: `0` to `3`.                          |

#### Example
```json
{
    "method": {
        "name": "VTCode",
        "vtCode": { "a": 1, "b": 2 }
    }
}
```

---

### 3. Differential VTCode Method

Generates candidates using **Differential VT (D-VT) codes**. These are based on the differential representation of the sequence.

```json
{
    "method": {
        "name": "Diff_VTCode",
        "diffVtCode": {
            "syndrome": 0
        }
    }
}
```

#### Parameters

| Key        | Type      | Default | Description                                                                                           |
| :--------- | :-------- | :------ | :---------------------------------------------------------------------------------------------------- |
| `syndrome` | `integer` | `0`     | The syndrome parameter `s` for the D-VT code constraint: `sum_{i=1}^{n} i * y_i ≡ s (mod n*4)`.      |

---

### 4. Random Method

Generates uniformly **random DNA strings**. Simple but less efficient than structured methods.

```json
{
    "method": {
        "name": "Random",
        "random": {
            "candidates": 50000
        }
    }
}
```

#### Parameters

| Key          | Type      | Default | Description                               |
| :----------- | :-------- | :------ | :---------------------------------------- |
| `candidates` | `integer` | `50000` | Number of random candidates to generate.  |

---

### 5. AllStrings Method

Enumerates **all 4^n possible DNA strings** of the given length. Only feasible for short lengths.

```json
{
    "method": {
        "name": "AllStrings"
    }
}
```

#### Parameters
None. This method has no additional parameters.

#### Feasibility Note
-   n=10: 4^10 = 1,048,576 candidates — feasible
-   n=12: 4^12 = 16,777,216 candidates — challenging
-   n=15: 4^15 = 1,073,741,824 candidates — not practical

---

### 6. FileRead Method

Reads candidates from an **external text file**. Useful for custom candidate sets or re-processing.

```json
{
    "method": {
        "name": "FileRead",
        "fileRead": {
            "input_file": "/path/to/candidates.txt"
        }
    }
}
```

#### Parameters

| Key          | Type     | Required | Description                                                                                         |
| :----------- | :------- | :------- | :-------------------------------------------------------------------------------------------------- |
| `input_file` | `string` | **Yes**  | Absolute path to the file containing candidate sequences. One sequence per line.                    |

#### File Format
-   One candidate per line
-   Supports both numeric format (`0123`) and DNA format (`ACGT`)
-   Lines not matching the expected length are automatically filtered out
-   Headers and empty lines are skipped

#### Example Input File
```
ACGTACGTACGT
TACGCATGCATG
GGCCTAATTGGC
```

---

## Complete Example

Here is a complete configuration file using all available options:

```json
{
    "dir": "output/full_example",
    "verify": true,

    "core": {
        "lenStart": 11,
        "lenEnd": 12,
        "editDist": 3
    },

    "constraints": {
        "maxRun": 3,
        "minGC": 0.3,
        "maxGC": 0.7
    },

    "performance": {
        "threads": 16,
        "saveInterval": 1800,
        "use_gpu": true,
        "max_gpu_memory_gb": 10.0
    },

    "clustering": {
        "enabled": true,
        "k": 200,
        "verbose": false,
        "convergenceIterations": 3
    },

    "method": {
        "name": "LinearCode",
        "linearCode": {
            "minHD": 3,
            "biasMode": "random",
            "rowPermMode": "identity",
            "colPermMode": "identity",
            "randomSeed": 12345
        }
    }
}
```

---

## Example Configurations

Ready-to-use example configurations are provided in the `config/examples/` directory:

| File                             | Description                                                    |
| :------------------------------- | :------------------------------------------------------------- |
| `all_params_reference.json`      | Annotated reference with all parameters and descriptions.      |
| `linear_code_ed3_basic.json`     | Basic LinearCode, edit distance 3, no constraints.             |
| `linear_code_ed4_constrained.json` | LinearCode, edit distance 4, GC and run constraints.          |
| `linear_code_ed5_gpu.json`       | LinearCode, edit distance 5, GPU enabled.                      |
| `linear_code_bias_perms.json`    | LinearCode with custom bias and permutations.                  |
| `linear_code_random_vectors.json` | LinearCode with random bias and permutations.                 |
| `cpu_only.json`                  | CPU-only configuration (no GPU).                               |
| `with_clustering.json`           | Configuration with clustering enabled.                         |
| `vtcode_basic.json`              | VTCode method example.                                         |
| `file_read_example.json`         | FileRead method example.                                       |

Copy any example to your working directory and modify as needed:

```bash
cp config/examples/linear_code_ed4_constrained.json my_config.json
./IndexGen --config my_config.json
```

---

## Tips and Best Practices

1.  **Start with LinearCode**: It's the most efficient method for most use cases. Begin with `minHD` equal to your target `editDist`.

2.  **Disable constraints for initial testing**: Set `maxRun: 0` and `minGC: 0, maxGC: 0` to see maximum possible codebook size.

3.  **Use GPU when available**: GPU acceleration provides 10-100x speedup for large candidate sets.

4.  **Experiment with random vectors**: Different biases and permutations can yield different codebook sizes. Use `random` modes with different seeds.

5.  **Monitor memory**: For very large candidate sets (>100,000), monitor GPU memory usage and reduce `max_gpu_memory_gb` if needed.

6.  **Save frequently for long runs**: Set `saveInterval` to 1800-3600 seconds (30-60 minutes) for multi-hour runs.
