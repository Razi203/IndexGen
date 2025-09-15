# DNA Codebook Generator

## 1. Project Overview

This project is a high-performance C++ application designed to generate robust sets of DNA sequences, known as "codebooks," for use in biotechnology applications like DNA data storage, genetic barcoding (e.g., in scRNA-seq), or DNA watermarking.

The primary goal is to create large codebooks where every sequence is significantly different from every other sequence, minimizing the chance of errors during DNA synthesis or sequencing. The key metric used is the **Levenshtein distance** (or edit distance), which accounts for the three main types of sequencing errors: insertions, deletions, and substitutions.

The core workflow involves:

1.  Generating a large pool of initial candidate DNA sequences that meet basic criteria.
2.  Constructing a "conflict graph" where sequences that are too similar (i.e., have an edit distance below a specified threshold) are connected.
3.  Using an efficient, greedy algorithm to find a **maximal independent set** in this graph. This set of unconnected sequences forms the final, error-resilient codebook.
4.  Providing tools to decode erroneous sequences back to their most likely original codeword.

The entire process is multi-threaded to leverage modern multi-core processors and includes a recovery mechanism for very long-running jobs.

---

## 2. File Structure

The project is organized into several distinct modules, each with a specific responsibility:

* `IndexGen (.hpp/.cpp)`: The main entry point of the application. It contains the `main()` function and the central `Params` struct used for configuration.
* `SparseMat (.hpp/.cpp)`: Implements the primary, memory-efficient algorithm for codebook generation using an adjacency list to represent the conflict graph and a greedy selection strategy.
* `Candidates (.hpp/.cpp)`: Responsible for generating the initial pool of candidate DNA sequences, with options for applying biological filters.
* `Decode (.hpp/.cpp)`: Implements the nearest-neighbor decoding algorithm to find the closest valid codeword for a given (potentially erroneous) sequence.
* `LinearCodes (.hpp/.cpp)`: A module for generating highly structured sets of candidates with a guaranteed minimum **Hamming distance** using linear block codes.
* `GF4 (.hpp/.cpp)`: Provides the mathematical foundation for `LinearCodes`, implementing arithmetic over the Galois Field of 4 elements (GF(4)), which corresponds to the four DNA bases {A, C, G, T}.
* `GenMat (.hpp/.cpp)`: A data module containing pre-computed generator matrices used by the `LinearCodes` module.
* `Utils (.hpp/.cpp)`: A collection of utility functions used across the project, including the core `FastEditDistance` calculation.
* `MaxClique (.hpp/.cpp)`: An alternative, more computationally intensive algorithm for codebook generation that solves the Maximum Clique problem. It can produce optimal results but is slower than `SparseMat`.
* `mcqd.h`: An external library providing a solver for the Maximum Clique problem.

---

## 3. Core Concepts

### Levenshtein vs. Hamming Distance

While **Hamming distance** only counts substitutions, **Levenshtein distance** counts insertions, deletions, and substitutions. Because DNA sequencing technologies are prone to insertion/deletion errors, using Levenshtein distance is critical for creating robust codebooks.



### The Generation Pipeline

The code generation follows a three-step pipeline:

**Step 1: Candidate Generation**
The process starts in the `Candidates` module. You can choose one of two methods:

* **Exhaustive:** Generate all 4^N possible DNA strings of a given length. This is thorough but computationally expensive.
* **Linear Codes:** Generate a smaller, structured subset of strings that already have a guaranteed minimum *Hamming distance*. This is much faster and is the recommended approach.

After generation, biological filters are applied to remove undesirable sequences, such as those with extreme GC-content or long homopolymer runs (e.g., "AAAAA").

**Step 2: Codebook Selection (Greedy Algorithm)**
This is the core of the `SparseMat` module.

1.  A "conflict graph" is built where each filtered candidate is a node.
2.  An edge is drawn between any two nodes if their Levenshtein distance is *less than* the desired minimum (`codeMinED`).
3.  The algorithm then iteratively selects a node with the fewest connections (lowest degree), adds it to the codebook, and removes it and all its neighbors from the graph.
4.  This process repeats until the graph is empty. The resulting set of selected nodes is the final codebook.

**Step 3: Decoding**
The `Decode` module is used after a codebook has been generated. If you sequence a DNA strand and get a result that isn't in your codebook (due to errors), the decoder performs a nearest-neighbor search to find the valid codeword with the minimum Levenshtein distance to your result.

---

## 4. How to Use the Project

### Step 1: Getting the Project Files

To use the project, you need to copy all the source and header files to a single directory on your local machine.

**Option A: Using Git (Recommended)**
If you have Git installed, you can clone the entire project repository with a single command:

```bash
git clone <repository_url>
cd <repository_directory>
````

*(Replace `<repository_url>` and `<repository_directory>` with the actual URL and folder name for the project.)*

**Option B: Manual Copy**
Create a new directory for the project and ensure all of the following files are present inside it:

```
Candidates.cpp
Candidates.hpp
Decode.cpp
Decode.hpp
GenMat.cpp
GenMat.hpp
GF4.cpp
GF4.hpp
IndexGen.cpp
IndexGen.hpp
LinearCodes.cpp
LinearCodes.hpp
MaxClique.cpp
MaxClique.hpp
mcqd.h
SparseMat.cpp
SparseMat.hpp
Utils.cpp
Utils.hpp
```

### Step 2: Compilation

You will need a C++ compiler (like g++). Navigate to the project directory and compile all the source files. Using optimization flags and enabling threading is highly recommended.

```bash
# Example compilation command on Linux/macOS
g++ -std=c++11 -O3 -pthread *.cpp -o dna_code_generator.exe
```

### Step 3: Configuration

All configuration is done inside the `main()` function in `IndexGen.cpp`. Open this file and edit the `Params` object.

```cpp
// Inside main() in IndexGen.cpp

// --- Configuration ---
int codeLen = 10;
int minHD = 4;
int minED = 4;
int maxRun = 3;
double minGCCont = 0.3;
double maxGCCont = 0.7;
int threadNum = 8;
int saveInterval = 80000;

Params params(codeLen, minHD, minED, maxRun, minGCCont, maxGCCont, threadNum, saveInterval);
```

**Parameter Guide:**

  * `codeLen`: The length of your DNA sequences.
  * `candMinHD`: **(Important)** The minimum Hamming distance for the initial candidates.
      * `1`: Brute-force, generates all 4^N strings. Very slow.
      * `2-5`: **Recommended.** Uses a linear code to generate a much smaller, pre-filtered set. A higher value gives better starting candidates but a smaller pool. `4` is a good starting point.
  * `codeMinED`: The target minimum Levenshtein distance for the final codebook. This is the main quality metric. A higher value gives more error resilience but results in a smaller codebook.
  * `maxRun`: The longest allowed homopolymer run. `3` or `4` is typical. Set to `0` to disable.
  * `minGCCont` / `maxGCCont`: The GC-content window (e.g., 0.3 to 0.7). Set both to `0` to disable.
  * `threadNum`: Set this to the number of CPU cores on your system for best performance.
  * `saveInterval`: How often (in seconds) to save progress. Necessary for long jobs.

### Step 4: Running the Generator

After compiling and configuring, run the executable from your terminal:

```bash
./dna_code_generator
```

The program will print its progress to the console. When finished, it will generate a `.txt` file in the same directory, named according to the code parameters (e.g., `CodeSize-0001234_CodeLen-10_MinED-4.txt`).

The output file contains a metadata header followed by the list of generated codewords, one per line.

### Step 5: Resuming a Job

If a long-running job is interrupted, you can resume it if `saveInterval` was set.

1.  Do **not** delete the `progress_*.txt` files created by the program.
2.  In `IndexGen.cpp`, comment out the `GenerateCodebookAdj(params);` line.
3.  Uncomment the `GenerateCodebookAdjResumeFromFile();` line.
4.  Recompile and run the program again. It will load the progress files and continue where it left off.

<!-- end list -->

```cpp
// --- Execution ---

// Comment out the original call
// for (int len = 10; len <= 16; len++) {
// 	 ...
// 	 GenerateCodebookAdj(params);
// 	 ...
// }

// Uncomment the resume call
GenerateCodebookAdjResumeFromFile();
```

```
```