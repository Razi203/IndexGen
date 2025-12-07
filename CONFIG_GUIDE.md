# IndexGen Configuration Guide

This guide explains all available options for the `config.json` file. The configuration is divided into several sections: global settings, core parameters, constraints, performance, and method-specific settings.

## Global Settings

| Key | Type | Description |
| :--- | :--- | :--- |
| `dir` | `string` | The output directory where results will be saved. Can be a nested path (e.g., `"Temp/Run1"`). If empty, a timestamp-based directory is created. |
| `verify` | `boolean` | If `true`, runs a verification step after generation to ensure all codewords satisfy the minimum edit distance constraint. Default: `false`. |

## Core Settings (`core`)

These parameters define the fundamental properties of the codebook.

| Key | Type | Description |
| :--- | :--- | :--- |
| `lenStart` | `integer` | The starting codeword length for the generation loop. |
| `lenEnd` | `integer` | The ending codeword length (inclusive). |
| `editDist` | `integer` | The minimum Levenshtein (edit) distance required between valid codewords. Common values: 3, 4. |

## Constraints (`constraints`)

Biological constraints applied to every candidate codeword.

| Key | Type | Description |
| :--- | :--- | :--- |
| `maxRun` | `integer` | The maximum length of a homopolymer run (e.g., `3` forbids `AAAA`). Set to `0` to disable. |
| `minGC` | `float` | The minimum GC content fraction (0.0 - 1.0). e.g., `0.3` means at least 30% G/C. |
| `maxGC` | `float` | The maximum GC content fraction (0.0 - 1.0). e.g., `0.7` means at most 70% G/C. |

## Performance (`performance`)

Settings to control execution speed and resource usage.

| Key | Type | Description |
| :--- | :--- | :--- |
| `threads` | `integer` | Number of CPU threads to use for parallel tasks. |
| `saveInterval` | `integer` | Interval (in seconds) to save progress checkpoints. |
| `use_gpu` | `boolean` | If `true`, uses the GPU-accelerated solver (requires Python environment). If `false`, uses the CPU-threaded solver. |
| `max_gpu_memory_gb` | `float` | Maximum GPU memory usage in GB. Passed to the Python script to optimize batch sizes. Default: `10.0`. |

## Generation Method (`method`)

Specifies the algorithm used to generate candidate strings.

| Key | Type | Description |
| :--- | :--- | :--- |
| `name` | `string` | The name of the method to use. Valid values: `"LinearCode"`, `"VTCode"`, `"Random"`, `"Diff_VTCode"`, `"AllStrings"`. |

### 1. LinearCode Method
Used when `"name": "LinearCode"`. Generates candidates using linear codes over GF(4).

| Key | Type | Description |
| :--- | :--- | :--- |
| `minHD` | `integer` | Minimum Hamming Distance for the linear code candidates. Valid values: 1-5. Higher values generate fewer candidates but faster. |
| `biasMode` | `string` | `"default"`, `"random"`, or `"manual"`. Controls the bias vector. |
| `rowPermMode` | `string` | `"identity"`, `"random"`, or `"manual"`. Controls row permutation. |
| `colPermMode` | `string` | `"identity"`, `"random"`, or `"manual"`. Controls column permutation. |
| `randomSeed` | `integer` | Seed for random vector generation. `0` uses current time. |
| `bias` | `array[int]` | Manual bias vector (GF(4) values 0-3). Required if `biasMode` is `"manual"`. |
| `rowPerm` | `array[int]` | Manual row permutation vector. Required if `rowPermMode` is `"manual"`. |
| `colPerm` | `array[int]` | Manual column permutation vector. Required if `colPermMode` is `"manual"`. |

### 2. VTCode Method
Used when `"name": "VTCode"`. Uses Varshamov-Tenengolts codes.

| Key | Type | Description |
| :--- | :--- | :--- |
| `a` | `integer` | Remainder parameter `a`. |
| `b` | `integer` | Remainder parameter `b`. |

### 3. Random Method
Used when `"name": "Random"`. Generates random strings.

| Key | Type | Description |
| :--- | :--- | :--- |
| `candidates` | `integer` | The number of random candidates to generate. |

### 4. Differential VTCode Method
Used when `"name": "Diff_VTCode"`.

| Key | Type | Description |
| :--- | :--- | :--- |
| `syndrome` | `integer` | The syndrome parameter. |

### 5. AllStrings Method
Used when `"name": "AllStrings"`. Generates all possible DNA strings of the given length. No additional parameters required.

---

## Example `config.json`

```json
{
    "dir": "output/run1",
    "core": {
        "lenStart": 10,
        "lenEnd": 10,
        "editDist": 3
    },
    "constraints": {
        "maxRun": 3,
        "minGC": 0.4,
        "maxGC": 0.6
    },
    "performance": {
        "threads": 16,
        "saveInterval": 3600,
        "use_gpu": true,
        "max_gpu_memory_gb": 12.0
    },
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
