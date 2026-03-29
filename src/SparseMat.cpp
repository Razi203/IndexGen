#include "SparseMat.hpp"
#include "Candidates.hpp"
#include "EditDistance.hpp"
#include "Utils.hpp"
#include "clustering/KMeansAdapter.hpp"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <climits>
#include <cstdlib> // Added for system()
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
/**
 * @brief Get the project root directory by detecting the executable's location.
 *
 * This function reads /proc/self/exe to find the running executable's path,
 * then navigates up to find the project root (where scripts/ directory exists).
 * Falls back to current directory if detection fails.
 */
std::string getProjectRoot()
{
    namespace fs = std::filesystem;

    try
    {
        // Read the symlink /proc/self/exe to get executable path
        fs::path exe_path = fs::read_symlink("/proc/self/exe");
        fs::path exe_dir = exe_path.parent_path();

        // Try to find src/ directory by looking in parent directories
        // Typically: /path/to/project/build/IndexGen -> /path/to/project/src
        fs::path search_path = exe_dir;
        for (int i = 0; i < 5; ++i)
        { // Look up to 5 levels up
            fs::path src_dir = search_path / "src";
            if (fs::exists(src_dir) && fs::is_directory(src_dir))
            {
                // Verify the GPU script exists
                if (fs::exists(src_dir / "gpu_graph_generator.py"))
                {
                    return search_path.string();
                }
            }
            if (search_path.has_parent_path() && search_path.parent_path() != search_path)
            {
                search_path = search_path.parent_path();
            }
            else
            {
                break;
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Warning: Could not auto-detect project root: " << e.what() << std::endl;
    }

    // Fall back to current directory
    return ".";
}
} // namespace

using namespace std;

// *** AdjList Member Function Implementations ***

void AdjList::Init(int numNodes)
{
    m.resize(numNodes);
    deleted.assign(numNodes, false);
    degree.assign(numNodes, 0);
    num_active_nodes = numNodes;
    min_degree_tracker = 0;
    // rowsBySum is allocated during RowsBySum() once degrees are known
}

void AdjList::RowsBySum()
{
    rowsBySum.clear();
    int max_d = 0;
    for (int i = 0; i < (int)m.size(); ++i)
    {
        if (deleted[i])
            continue;
        int d = m[i].size();
        degree[i] = d;
        if (d > max_d)
            max_d = d;
    }

    rowsBySum.resize(max_d + 1);
    min_degree_tracker = max_d + 1;

    for (int i = 0; i < (int)m.size(); ++i)
    {
        if (deleted[i])
            continue;
        int d = degree[i];
        rowsBySum[d].insert(i);
        if (d < min_degree_tracker)
            min_degree_tracker = d;
    }
}

int AdjList::MinSumRow() const
{
    assert(min_degree_tracker >= 0 && min_degree_tracker < (int)rowsBySum.size());
    assert(!rowsBySum[min_degree_tracker].empty());
    return *rowsBySum[min_degree_tracker].begin();
}

int AdjList::MaxSumRow() const
{
    for (int d = (int)rowsBySum.size() - 1; d >= 0; --d)
    {
        if (!rowsBySum[d].empty())
            return *rowsBySum[d].begin();
    }
    return -1;
}

void AdjList::DeleteRow(const int currentSum, const int row)
{
    rowsBySum[currentSum].erase(row);
    // Note: min_degree_tracker may need to advance, but this is handled lazily in MinSumRow calls or explicitly in
    // DecreaseSum
    if (currentSum == min_degree_tracker && rowsBySum[currentSum].empty())
    {
        // Advance min_degree_tracker
        while (min_degree_tracker < (int)rowsBySum.size() && rowsBySum[min_degree_tracker].empty())
        {
            min_degree_tracker++;
        }
    }
}

void AdjList::DecreaseSum(const int currentSum, const int row)
{
    assert(currentSum > 0);
    DeleteRow(currentSum, row);
    degree[row] = currentSum - 1;
    rowsBySum[currentSum - 1].insert(row);
    if (currentSum - 1 < min_degree_tracker)
    {
        min_degree_tracker = currentSum - 1;
    }
}

int AdjList::RemoveEmptyRows()
{
    int removedRowsNum = 0;
    if (rowsBySum.size() > 0 && !rowsBySum[0].empty())
    {
        // Make a copy since we will delete them
        std::vector<int> zeros(rowsBySum[0].begin(), rowsBySum[0].end());
        removedRowsNum = zeros.size();
        for (int row : zeros)
        {
            deleted[row] = true;
            num_active_nodes--;
            rowsBySum[0].erase(row);
        }
        while (min_degree_tracker < (int)rowsBySum.size() && rowsBySum[min_degree_tracker].empty())
        {
            min_degree_tracker++;
        }
    }
    return removedRowsNum;
}

bool AdjList::empty() const
{
    return num_active_nodes == 0;
}

int AdjList::RowNum() const
{
    return num_active_nodes;
}

void AdjList::Set(int row, int col)
{
    if (row >= (int)m.size())
        return; // Protection
    m[row].push_back(col);
    // degree logic is initialized later via RowsBySum()
}

void AdjList::DelRowCol(int i)
{
    if (deleted[i])
        return;

    const std::vector<int> &js = m[i];
    for (int j : js)
    {
        if (deleted[j])
            continue;
        int currRowJSum = degree[j];
        if (currRowJSum > 0)
        {
            DecreaseSum(currRowJSum, j);
        }
    }

    DeleteRow(degree[i], i);
    deleted[i] = true;
    num_active_nodes--;
}

void AdjList::DelBall(const int matRow, unordered_set<int> &remaining)
{
    if (deleted[matRow])
        return;

    // Collect all elements to logically delete: the node itself and all its active neighbors
    vector<int> toDel;
    const std::vector<int> &neighbors = m[matRow];
    for (int num : neighbors)
    {
        if (!deleted[num])
        {
            toDel.push_back(num);
        }
    }
    toDel.push_back(matRow);

    for (int num : toDel)
    {
        DelRowCol(num);
        remaining.erase(num);
    }
}

int AdjList::FindMinDel(unordered_set<int> &remaining, double &minSumRowTime, double &delBallTime)
{
    auto msr_start = chrono::steady_clock::now();
    int minSumRow = MinSumRow();
    auto msr_end = chrono::steady_clock::now();
    minSumRowTime += chrono::duration<double>(msr_end - msr_start).count();

    auto db_start = chrono::steady_clock::now();
    DelBall(minSumRow, remaining);
    auto db_end = chrono::steady_clock::now();
    delBallTime += chrono::duration<double>(db_end - db_start).count();

    return minSumRow;
}

int AdjList::FindMaxDel(unordered_set<int> &remaining, double &maxSumRowTime, double &delRowColTime)
{
    auto msr_start = chrono::steady_clock::now();
    int maxSumRow = MaxSumRow();
    auto msr_end = chrono::steady_clock::now();
    maxSumRowTime += chrono::duration<double>(msr_end - msr_start).count();

    auto drc_start = chrono::steady_clock::now();
    DelRowCol(maxSumRow);
    remaining.erase(maxSumRow);
    auto drc_end = chrono::steady_clock::now();
    delRowColTime += chrono::duration<double>(drc_end - drc_start).count();

    RemoveEmptyRows();

    return maxSumRow;
}

void AdjList::ToFile(const string &filename) const
{
    ofstream output;
    output.open(filename.c_str());
    if (!output.is_open())
    {
        std::cout << "Failed opening output file!" << endl;
        return;
    }
    for (int i = 0; i < (int)m.size(); i++)
    {
        if (deleted[i])
            continue;
        for (int j : m[i])
        {
            if (!deleted[j])
                output << i << '\t' << j << '\n';
        }
    }
    output.close();
}
void AdjList::FromFile(const string &filename)
{
    ifstream input;
    input.open(filename.c_str());
    if (!input.is_open())
    {
        std::cout << "Failed opening input file!" << endl;
        return;
    }
    int a, b;
    int max_node = -1;
    vector<pair<int, int>> edges;
    while (input >> a >> b)
    {
        edges.push_back({a, b});
        if (a > max_node)
            max_node = a;
        if (b > max_node)
            max_node = b;
    }
    input.close();

    if (max_node >= 0)
        Init(max_node + 1);
    for (const auto &e : edges)
    {
        m[e.first].push_back(e.second);
    }
}

// *** NEW: FAST BINARY LOADER FOR GPU INTEGRATION ***
void AdjList::FromBinaryFile(const string &filename, long long int &matrixOnesNum, bool silent)
{
    ifstream input(filename, ios::binary);
    if (!input.is_open())
    {
        std::cerr << "Error: Could not open binary edges file: " << filename << endl;
        return;
    }

    // Read file into a buffer
    input.seekg(0, ios::end);
    size_t fileSize = input.tellg();
    input.seekg(0, ios::beg);

    if (fileSize == 0)
        return;

    std::vector<int32_t> buffer(fileSize / sizeof(int32_t));
    input.read(reinterpret_cast<char *>(buffer.data()), fileSize);
    input.close();

    // Process buffer (format: u, v, u, v...)
    // Buffer contains only UNIQUE pairs (u < v). We must add symmetric edges.
    size_t numInts = buffer.size();
    for (size_t i = 0; i < numInts; i += 2)
    {
        int u = buffer[i];
        int v = buffer[i + 1];
        if (u >= (int)m.size() || v >= (int)m.size())
            continue;
        m[u].push_back(v);
        m[v].push_back(u);
    }
    matrixOnesNum = numInts;
    if (!silent)
        std::cout << "Loaded " << (numInts / 2) << " edges from binary file." << endl;

    // Shrink memory instantly by freeing excess vector capacities
    for (auto &vec : m)
    {
        vec.shrink_to_fit();
    }
}

// *** Standalone Helper Functions ***

void PairsToFile(const string &filename, const vector<pair<int, int>> &vec)
{
    ofstream output;
    output.open(filename.c_str());
    if (!output.is_open())
    {
        std::cout << "Failed opening output file!" << endl;
        return;
    }
    for (const pair<int, int> &pr : vec)
    {
        output << pr.first << '\t' << pr.second << '\n';
    }
    output.close();
}

void PairsFromFile(const string &filename, vector<pair<int, int>> &vec)
{
    ifstream input;
    input.open(filename.c_str());
    if (!input.is_open())
    {
        std::cout << "Failed opening input file!" << endl;
        return;
    }
    int a, b;
    while (input >> a >> b)
    {
        pair<int, int> curr = make_pair(a, b);
        vec.push_back(curr);
    }
    input.close();
}

void SaveProgressAdjListComp(const int i, const vector<pair<int, int>> &threadPairVec, const int threadIdx)
{
    string alFilename = "progress_adj_list_comp_" + to_string(threadIdx) + ".txt";
    PairsToFile(alFilename, threadPairVec);
    string iFilename = "progress_adj_list_comp_i_" + to_string(threadIdx) + ".txt";
    NumToFile(i, iFilename);
}

void LoadProgressAdjListComp(int &i, vector<pair<int, int>> &threadPairVec, int threadIdx)
{
    string alFilename = "progress_adj_list_comp_" + to_string(threadIdx) + ".txt";
    PairsFromFile(alFilename, threadPairVec);
    string iFilename = "progress_adj_list_comp_i_" + to_string(threadIdx) + ".txt";
    FileToNum(i, iFilename);
}

void DelProgressAdjListComp(const int threadIdx)
{
    string alFilename = "progress_adj_list_comp_" + to_string(threadIdx) + ".txt";
    remove(alFilename.c_str());
    string iFilename = "progress_adj_list_comp_i_" + to_string(threadIdx) + ".txt";
    remove(iFilename.c_str());
}

// *** NEW: Helper to run Python GPU script (System Call) ***
// OPTIMIZATION: Added inputFilename argument to avoid re-writing the vector file
void FillAdjListGPU(AdjList &adjList, const vector<string> &candidates, const int minED, long long int &matrixOnesNum,
                    const string &inputFilename = "", double maxGPUMemoryGB = 10.0, bool silent = false, bool isBinary = false)
{
    static std::atomic<int> file_id{0};
    int id = file_id++;
    string vecFile = "temp_vectors_" + to_string(id) + ".txt";
    string edgesFile = "temp_edges_" + to_string(id) + ".bin";

    string fileToUse = vecFile;

    // OPTIMIZATION: If we already have a file on disk (progress_cand.txt), use it!
    if (!inputFilename.empty())
    {
        if (!silent)
            std::cout << "[C++] Using existing candidate file: " << inputFilename << endl;
        fileToUse = inputFilename;
    }
    else
    {
        // Fallback: Save candidates to temp file
        if (!silent)
            std::cout << "[C++] Saving candidates to " << vecFile << "..." << endl;
        StrVecToFile(candidates, vecFile);
    }

    // 2. Call Python Script
    // Usage: python3 gpu_graph_generator.py <input> <output> <threshold>
    // Determine script path: auto-detect from executable location, allow env override
    const char *env_root = std::getenv("INDEXGEN_ROOT");
    string project_root = (env_root) ? string(env_root) : getProjectRoot();
    string script_path = project_root + (isBinary ? "/src/gpu_graph_generator_binary.py" : "/src/gpu_graph_generator.py");

    string cmd = "python " + script_path + " " + fileToUse + " " + edgesFile + " " + to_string(minED) + " " +
                 to_string(maxGPUMemoryGB);

    if (silent)
    {
        cmd += " > /dev/null 2>&1";
    }
    else
    {
        std::cout << "[C++] Executing GPU Solver: " << cmd << endl;
    }

    int ret = system(cmd.c_str());
    if (ret != 0)
    {
        if (silent)
        {
            // If it failed silently, try to run again without silence to show error, or just print generic error
            std::cerr << "Error: Python GPU script failed (run without silence to see details)." << endl;
        }
        else
        {
            std::cerr << "Error: Python GPU script failed." << endl;
        }
        exit(1);
    }

    // 3. Load Binary Edges
    if (!silent)
        std::cout << "[C++] Loading edges from " << edgesFile << "..." << endl;
    auto load_start = chrono::steady_clock::now();

    adjList.Init(candidates.size());
    adjList.FromBinaryFile(edgesFile, matrixOnesNum, silent);

    auto load_end = chrono::steady_clock::now();
    chrono::duration<double> load_time = load_end - load_start;
    if (!silent)
        std::cout << "[C++] Edge load time: " << fixed << setprecision(2) << load_time.count() << "s" << endl;

    // Clean up temp file only if we created it
    if (inputFilename.empty())
    {
        remove(vecFile.c_str());
    }
    remove(edgesFile.c_str());
}

void FillAdjListTH(vector<pair<int, int>> &pairVec, const vector<string> &candidates,
                   const vector<vector<char>> &cand0123Cont, const int minED, const int threadStart,
                   const int threadIdx, const int threadNum, const int saveInterval)
{
    auto lastSaveTime = chrono::steady_clock::now();
    int candNum = candidates.size();
    for (int i = threadStart; i < candNum; i += threadNum)
    {
        PatternHandle H = MakePattern(candidates[i]);
        for (int j = i + 1; j < candNum; j++)
        {
            bool EDIsAtLeastMinED =
                // FastEditDistance0123(candidates[i], candidates[j], minED, cand0123Cont[i], cand0123Cont[j]);
                // EditDistanceExactAtLeast(candidates[j], H, minED);
                EditDistanceBandedAtLeast(candidates[j], H, minED);
            if (!EDIsAtLeastMinED)
            {
                pairVec.push_back(make_pair(i, j));
            }
        }
        auto currentTime = chrono::steady_clock::now();
        chrono::duration<double> elapsed_seconds = currentTime - lastSaveTime;
        if (saveInterval > 0 && elapsed_seconds.count() > saveInterval)
        {
            SaveProgressAdjListComp(i, pairVec, threadIdx);
            lastSaveTime = currentTime;
            std::cout << "Adj List Comp PROGRESS: i=" << i << " of " << candNum << "\tthreadId\t" << threadIdx << endl;
        }
    }
}

void FillAdjList(AdjList &adjList, const vector<string> &candidates, const int minED, const int threadNum,
                 const int saveInterval, const bool resume, long long int &matrixOnesNum)
{
    vector<vector<pair<int, int>>> threadPairVecs(threadNum, vector<pair<int, int>>());
    vector<thread> threads;
    vector<int> threadStartCand(threadNum);
    vector<vector<char>> cand0123Cont = Cont0123(candidates);

    if (not resume)
    {
        for (int i = 0; i < threadNum; i++)
        {
            threadStartCand[i] = i;
        }
    }
    else
    {
        for (int i = 0; i < threadNum; i++)
        {
            LoadProgressAdjListComp(threadStartCand[i], threadPairVecs[i], i);
            threadStartCand[i] += threadNum;
        }
    }

    for (int i = 0; i < threadNum; i++)
    {
        threads.push_back(thread(FillAdjListTH, ref(threadPairVecs[i]), ref(candidates), ref(cand0123Cont), minED,
                                 threadStartCand[i], i, threadNum, saveInterval));
    }
    for (thread &th : threads)
        th.join();

    adjList.Init(candidates.size());
    matrixOnesNum = 0;
    for (vector<pair<int, int>> &thvec : threadPairVecs)
    {
        matrixOnesNum += (thvec.size() * 2);
        for (pair<int, int> &pr : thvec)
        {
            adjList.Set(pr.first, pr.second);
            adjList.Set(pr.second, pr.first);
        }
    }
    for (int i = 0; i < threadNum; i++)
    {
        DelProgressAdjListComp(i);
    }
    adjList.RowsBySum();
}

void IndicesToSet(unordered_set<int> &remaining, int indexNum)
{
    for (int i = 0; i < indexNum; i++)
    {
        remaining.insert(i);
    }
}

void USetIntToFile(const unordered_set<int> &uSet, const string &filename)
{
    ofstream output;
    output.open(filename.c_str());
    if (!output.is_open())
    {
        std::cout << "Failed opening output file!" << std::endl;
        return;
    }
    for (const int &num : uSet)
    {
        output << num << '\n';
    }
    output.close();
}

void USetIntFromFile(unordered_set<int> &uSet, const string &filename)
{
    ifstream input;
    input.open(filename.c_str());
    if (!input.is_open())
    {
        std::cout << "Failed opening input file!" << std::endl;
        return;
    }
    int a;
    while (input >> a)
    {
        uSet.insert(a);
    }
    input.close();
}

void SaveProgressCodebook(const unordered_set<int> &remaining, const AdjList &adjList, const vector<string> &codebook)
{
    USetIntToFile(remaining, "progress_remaining.txt");
    StrVecToFile(codebook, "progress_codebook.txt");
    adjList.ToFile("progress_adj_list.txt");
}

void LoadProgressCodebook(unordered_set<int> &remaining, AdjList &adjList, vector<string> &codebook)
{
    USetIntFromFile(remaining, "progress_remaining.txt");
    FileToStrVec(codebook, "progress_codebook.txt");
    adjList.FromFile("progress_adj_list.txt");
}

void DelProgressCodebook()
{
    remove("progress_remaining.txt");
    remove("progress_codebook.txt");
    remove("progress_adj_list.txt");
}

void Codebook(AdjList &adjList, vector<string> &codebook, const vector<string> &candidates, const int saveInterval,
              const bool resume)
{
    codebook.clear();
    auto lastSaveTime = chrono::steady_clock::now();

    unordered_set<int> remaining;
    if (not resume)
    {
        IndicesToSet(remaining, candidates.size());
        SaveProgressCodebook(remaining, adjList, codebook);
    }
    else
    {
        LoadProgressCodebook(remaining, adjList, codebook);
    }

    double minSumRowTime = 0.0, delBallTime = 0.0;

    while (not adjList.empty())
    {
        // TODO: Replace with OOP design
        // Uncomment the method for which to filter the candidates

        // // (1) Choose min sum row and delete its ball
        int minEntry = adjList.FindMinDel(remaining, minSumRowTime, delBallTime);
        codebook.push_back(candidates[minEntry]);

        // (2) Remove max sum row candidate without adding to codebook
        // adjList.FindMaxDel(remaining, minSumRowTime, delBallTime);

        auto currentTime = chrono::steady_clock::now();
        chrono::duration<double> elapsed_seconds = currentTime - lastSaveTime;

        if (saveInterval > 0 && elapsed_seconds.count() > saveInterval)
        {
            SaveProgressCodebook(remaining, adjList, codebook);
            lastSaveTime = currentTime;
            std::cout << "Codebook PROGRESS: Remaining Rows " << adjList.RowNum() << std::endl;
        }
    }

    std::cout << "Find Min Sum Row Time:\t" << fixed << setprecision(2) << minSumRowTime << "\tseconds" << std::endl;
    std::cout << "Del Ball Time:\t\t" << fixed << setprecision(2) << delBallTime << "\tseconds" << std::endl;

    // Once adjList is empty (no edges left), add all remaining vertices to codebook
    for (int num : remaining)
    {
        codebook.push_back(candidates[num]);
    }
    DelProgressCodebook();
}

// Updated signature to include useGPU flag and isBinary flag
void CodebookAdjList(const vector<string> &candidates, vector<string> &codebook, const int minED, const int threadNum,
                     const int saveInterval, long long int &matrixOnesNum,
                     std::chrono::duration<double> &fillAdjListTime, std::chrono::duration<double> &processMatrixTime,
                     const bool useGPU, double maxGPUMemoryGB, const string &candFilename = "", bool isBinary = false) // ADDED Arguments
{
    AdjList adjList;
    NumToFile(1, "progress_stage.txt");

    auto starta = chrono::steady_clock::now();

    if (useGPU)
    {
        // --- INTEGRATION CHANGE: USE GPU SOLVER ---
        // Pass the existing filename (candFilename) to avoid redundant writing
        std::cout << "[C++] Mode: GPU Accelerated (Max Mem: " << maxGPUMemoryGB << " GB) (Binary: " << isBinary << ")" << endl;
        FillAdjListGPU(adjList, candidates, minED, matrixOnesNum, candFilename, maxGPUMemoryGB, false, isBinary);
    }
    else
    {
        // --- ORIGINAL: CPU THREADED SOLVER ---
        std::cout << "[C++] Mode: CPU Threaded" << endl;
        FillAdjList(adjList, candidates, minED, threadNum, saveInterval, false, matrixOnesNum);
    }

    adjList.RowsBySum(); // Must build the optimization map after loading
    // ------------------------------------------

    auto enda = chrono::steady_clock::now();
    fillAdjListTime = enda - starta;
    std::cout << "Fill AdjList Time:\t" << fixed << setprecision(2) << fillAdjListTime.count() << "\tseconds" << endl;

    NumToFile(2, "progress_stage.txt");
    LongLongIntToFile(matrixOnesNum, "matrix_ones_num.txt");

    auto startc = chrono::steady_clock::now();
    Codebook(adjList, codebook, candidates, saveInterval, false);
    auto endc = chrono::steady_clock::now();
    processMatrixTime = endc - startc;
    std::cout << "Process Matrix Time:\t" << fixed << setprecision(2) << processMatrixTime.count() << "\tseconds"
              << endl;

    remove("progress_stage.txt");
    remove("matrix_ones_num.txt");
}

/**
 * @brief Solves the Independent Set problem for a given set of candidates.
 * @return A subset of candidates forming the codebook.
 */
// Optimized version that reuses the GPU logic if enabled
std::vector<std::string> SolveIndependentSet(const std::vector<std::string> &candidates, const int minED,
                                             const int threadNum, const bool useGPU, double maxGPUMemoryGB, bool isBinary = false)
{
    // If empty or trivial
    if (candidates.empty())
        return {};
    if (candidates.size() == 1)
        return candidates;

    AdjList adjList;
    long long int matrixOnesNum = 0;

    // Fill AdjList (Graph Construction)
    // We use a temporary filename for GPU interaction if needed
    // Use a unique name to avoid collision if running in parallel logic
    static std::atomic<int> call_count{0};
    string candFilename = "temp_cand_" + to_string(call_count++) + ".txt";

    // We need to write candidates to file for GPU logic
    if (useGPU)
    {
        StrVecToFile(candidates, candFilename);
    }

    if (useGPU)
    {
        // Suppress some output during inner loops
        // std::cout << "..." << endl;
        FillAdjListGPU(adjList, candidates, minED, matrixOnesNum, candFilename, maxGPUMemoryGB, true, isBinary);
    }
    else
    {
        FillAdjList(adjList, candidates, minED, threadNum, 0, false, matrixOnesNum);
    }

    adjList.RowsBySum();

    // Solve Codebook (Max Independent Set / Min Vertex Cover on Complement)
    // Using existing Codebook function logic but without persistence/files for inner loops ideally
    // But existing Codebook function relies on persistence?
    // Let's copy the core logic of `Codebook` here but simplified for memory-only operation
    // OR just use the existing `Codebook` function and cleanup files.

    // Optimized memory-only version of "Codebook" function logic:
    std::vector<std::string> result_codebook;
    unordered_set<int> remaining;
    IndicesToSet(remaining, candidates.size());

    double d1 = 0, d2 = 0;
    while (!adjList.empty())
    {
        int minEntry = adjList.FindMinDel(remaining, d1, d2);
        result_codebook.push_back(candidates[minEntry]);
    }
    for (int num : remaining)
    {
        result_codebook.push_back(candidates[num]);
    }

    // Cleanup
    if (useGPU)
    {
        remove(candFilename.c_str());
    }

    return result_codebook;
}

void CodebookAdjListResumeFromFile(const vector<string> &candidates, vector<string> &codebook, const Params &params,
                                   long long int &matrixOnesNum)
{
    AdjList adjList;
    int stage;
    FileToNum(stage, "progress_stage.txt");
    if (stage == 1)
    {
        std::cout << "Resuming adj list comp" << endl;
        // Resume logic currently uses CPU. If restart is needed, it will use GPU from scratch.
        FillAdjList(adjList, candidates, params.codeMinED, params.threadNum, params.saveInterval, true, matrixOnesNum);
        NumToFile(2, "progress_stage.txt");
        Codebook(adjList, codebook, candidates, params.saveInterval, false);
    }
    else
    {
        assert(stage == 2);
        std::cout << "Resuming codebook comp" << endl;
        FileToLongLongInt(matrixOnesNum, "matrix_ones_num.txt");
        Codebook(adjList, codebook, candidates, params.saveInterval, true);
        remove("matrix_ones_num.txt");
    }
    remove("progress_stage.txt");
}

void GenerateCodebookAdj(const Params &params)
{
    auto start = std::chrono::steady_clock::now();
    ParamsToFile(params, "progress_params.txt");
    PrintTestParams(params);

    auto start_candidates = std::chrono::steady_clock::now();
    vector<string> candidates = Candidates(params);
    std::cout << "Number of Candidates: " << NumberWithCommas(candidates.size()) << std::endl;

    // This file is saved here for checkpointing.
    // We will reuse this filename for the GPU input to avoid re-writing 262k strings.
    string candFilename = "progress_cand.txt";
    StrVecToFile(candidates, candFilename);

    auto end_candidates = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_secs_candidates = end_candidates - start_candidates;
    std::cout << "Candidates Time: " << fixed << setprecision(2) << elapsed_secs_candidates.count() << "\tseconds"
              << std::endl;

    std::chrono::duration<double> fillAdjListTime, processMatrixTime;

    vector<string> codebook;
    long long int matrixOnesNum;

    // --- REFACTORED LOGIC FOR CLUSTERING ---
    int final_iteration = 0; // Track iterations for clustering
    double totalClusteringTime = 0.0;
    double totalSolvingTime = 0.0;

    if (!params.clustering.enabled)
    {
        // OLD BEHAVIOR: One pass
        std::cout << "Clustering disabled. Running standard generation..." << std::endl;
        bool isBinary = (params.method == GenerationMethod::LINEAR_BINARY_CODE);
        // Pass candFilename to the function
        CodebookAdjList(candidates, codebook, params.codeMinED, params.threadNum, params.saveInterval, matrixOnesNum,
                        fillAdjListTime, processMatrixTime, params.useGPU, params.maxGPUMemoryGB, candFilename, isBinary);
    }
    else
    {
        // NEW BEHAVIOR: Iterative Clustering
        std::cout << "Clustering enabled. Starting iterative process..." << std::endl;

        std::vector<std::string> current_candidates = candidates;
        std::vector<size_t> previous_sizes;
        int iteration = 0;

        while (true)
        {
            auto iter_start = std::chrono::steady_clock::now();
            iteration++;
            std::cout << "Starting Iteration " << iteration
                      << ": Candidate set size = " << NumberWithCommas(current_candidates.size()) << std::endl;

            // Step 1: Cluster
            auto cluster_start = std::chrono::steady_clock::now();

            // Check trivial case
            if (current_candidates.empty())
                break;

            // Dynamic cluster sizing: enforce max_cluster_size ceiling of 200K
            const int MAX_CLUSTER_SIZE = 200000;
            int effective_k = params.clustering.k;
            int N = current_candidates.size();
            if (N / effective_k > MAX_CLUSTER_SIZE)
            {
                effective_k = std::max(2, (N + MAX_CLUSTER_SIZE - 1) / MAX_CLUSTER_SIZE);
                std::cout << "[Auto] Overriding K from " << params.clustering.k << " to " << effective_k
                          << " (max cluster size = " << MAX_CLUSTER_SIZE << ")" << std::endl;
            }

            bool isBinary = (params.method == GenerationMethod::LINEAR_BINARY_CODE);
            // Configure KMeansAdapter with user config method
            indexgen::clustering::KMeansAdapter adapter(effective_k, params.clustering.method, isBinary);
            std::vector<std::vector<std::string>> clusters = adapter.cluster(current_candidates);

            std::cout << "Clustering produced " << clusters.size() << " clusters." << std::endl;

            auto cluster_end = std::chrono::steady_clock::now();
            double clustering_time = std::chrono::duration<double>(cluster_end - cluster_start).count();

            // Step 2: Solve Independent Set per Cluster
            std::vector<std::string> next_candidates;
            std::vector<std::vector<std::string>> cluster_results(clusters.size());

            auto solve_start = std::chrono::steady_clock::now();

            // We'll collect individual times for average
            std::vector<double> individual_solve_times;
            individual_solve_times.reserve(clusters.size());

            int num_clusters = clusters.size();
            std::atomic<int> next_cluster{0};
            std::mutex times_mutex;

            int num_concurrent = std::min((int)clusters.size(), params.threadNum);
            double mem_limit = params.maxGPUMemoryGB;
            if (params.useGPU)
            {
                // To avoid OOM, divide raw memory proportionally
                mem_limit = std::max(0.5, params.maxGPUMemoryGB / num_concurrent);
            }

            auto worker_func = [&]()
            {
                while (true)
                {
                    int i = next_cluster++;
                    if (i >= num_clusters)
                        break;

                    auto single_solve_start = std::chrono::steady_clock::now();

                    int threads_for_this = 1;
                    if (!params.useGPU)
                    {
                        threads_for_this = std::max(1, params.threadNum / num_concurrent);
                    }

                    cluster_results[i] =
                        SolveIndependentSet(clusters[i], params.codeMinED, threads_for_this, params.useGPU, mem_limit, isBinary);

                    auto single_solve_end = std::chrono::steady_clock::now();
                    double duration = std::chrono::duration<double>(single_solve_end - single_solve_start).count();

                    std::lock_guard<std::mutex> lock(times_mutex);
                    individual_solve_times.push_back(duration);
                }
            };

            std::vector<std::thread> workers;
            for (int t = 0; t < num_concurrent; ++t)
            {
                workers.emplace_back(worker_func);
            }
            for (auto &w : workers)
            {
                w.join();
            }

            auto solve_end = std::chrono::steady_clock::now();
            double solving_total_time = std::chrono::duration<double>(solve_end - solve_start).count();

            totalClusteringTime += clustering_time;
            totalSolvingTime += solving_total_time;

            // Step 3: Combine
            for (const auto &res : cluster_results)
            {
                next_candidates.insert(next_candidates.end(), res.begin(), res.end());
            }

            // Step 4: Check Convergence (3 iterations identical size)
            previous_sizes.push_back(next_candidates.size());

            auto iter_end = std::chrono::steady_clock::now();
            double iteration_total_time = std::chrono::duration<double>(iter_end - iter_start).count();

            // VERBOSE OUTPUT
            if (params.clustering.verbose)
            {
                double avg_solve_time = 0.0;
                if (!individual_solve_times.empty())
                {
                    double sum = 0;
                    for (auto t : individual_solve_times)
                        sum += t;
                    avg_solve_time = sum / individual_solve_times.size();
                }

                std::cout << "[Verbose] Iteration " << iteration << " Stats:" << std::endl;
                std::cout << "  - Total Iteration Time: " << fixed << setprecision(3) << iteration_total_time << " s"
                          << std::endl;
                std::cout << "  - Clustering Time:      " << fixed << setprecision(3) << clustering_time << " s"
                          << std::endl;
                std::cout << "  - Total Solving Time:   " << fixed << setprecision(3) << solving_total_time << " s"
                          << std::endl;
                std::cout << "  - Avg Cluster Solve:    " << fixed << setprecision(3) << avg_solve_time << " s"
                          << std::endl;
            }

            int converge_n = params.clustering.convergenceIterations;
            if (previous_sizes.size() >= (size_t)converge_n)
            {
                size_t n = previous_sizes.size();
                bool converged = true;
                for (int k = 1; k < converge_n; ++k)
                {
                    if (previous_sizes[n - 1] != previous_sizes[n - 1 - k])
                    {
                        converged = false;
                        break;
                    }
                }

                if (converged)
                {
                    std::cout << "Convergence reached! Sizes: ";
                    for (int k = converge_n; k >= 1; --k)
                    {
                        std::cout << previous_sizes[n - k] << (k > 1 ? " -> " : "");
                    }
                    std::cout << std::endl;

                    codebook = next_candidates;
                    final_iteration = iteration;
                    break;
                }
            }

            // Update for next iteration
            current_candidates = next_candidates;
        }
    }

    long long int candidateNum = candidates.size(); // Original candidates count
    // matrixOnesNum is only valid for the single pass non-clustering version in the old logic.
    // For clustering, we don't have a single "global" matrix ones count.
    int clusterK = -1;
    int clusterIterations = -1;
    if (params.clustering.enabled)
    {
        matrixOnesNum = 0;
        clusterK = params.clustering.k;
        clusterIterations = final_iteration;
    }

    PrintTestResults(candidateNum, matrixOnesNum, codebook.size(), clusterK, clusterIterations,
                     params.clustering.convergenceIterations, params.clustering.method);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> overAllTime = end - start;

    // For clustering: pass accumulated times as fillAdjListTime (clustering) and processMatrixTime (solving)
    if (params.clustering.enabled)
    {
        fillAdjListTime = std::chrono::duration<double>(totalClusteringTime);
        processMatrixTime = std::chrono::duration<double>(totalSolvingTime);
    }

    ToFile(codebook, params, candidateNum, matrixOnesNum, elapsed_secs_candidates, fillAdjListTime, processMatrixTime,
           overAllTime, clusterK, clusterIterations);
    if (params.verify)
    {
        bool isBinary = (params.method == GenerationMethod::LINEAR_BINARY_CODE);
        VerifyDist(codebook, params.codeMinED, params.threadNum, params.useGPU, params.maxGPUMemoryGB, isBinary);
    }
    std::cout << "=====================================================" << std::endl;
    remove("progress_params.txt");
    remove("progress_cand.txt");

    if (params.clustering.enabled && final_iteration > 0)
    {
        std::cout << "Total Clustering Time:\t" << fixed << setprecision(2) << totalClusteringTime << "\tseconds"
                  << " (avg " << fixed << setprecision(2) << totalClusteringTime / final_iteration << " s/iter)"
                  << std::endl;
        std::cout << "Total Solving Time:\t" << fixed << setprecision(2) << totalSolvingTime << "\tseconds"
                  << " (avg " << fixed << setprecision(2) << totalSolvingTime / final_iteration << " s/iter)"
                  << std::endl;
    }
    std::cout << "Codebook Time: " << fixed << setprecision(2) << overAllTime.count() << "\tseconds" << std::endl;
    std::cout << "=====================================================" << std::endl;
}

void GenerateCodebookAdjResumeFromFile()
{
    try
    {
        Params params;
        FileToParams(params, "progress_params.txt");
        std::cout << "Resuming Codebook Adj from file" << std::endl;
        PrintTestParams(params);
        vector<string> candidates;
        FileToStrVec(candidates, "progress_cand.txt");
        vector<string> codebook;
        int candidateNum = candidates.size();
        long long int matrixOnesNum;
        CodebookAdjListResumeFromFile(candidates, codebook, params, matrixOnesNum);
        if (params.clustering.enabled)
        {
            PrintTestResults(candidateNum, matrixOnesNum, codebook.size(), params.clustering.k, -1,
                             params.clustering.convergenceIterations, params.clustering.method);
        }
        else
        {
            PrintTestResults(candidateNum, matrixOnesNum, codebook.size());
        }
        ToFile(codebook, params, candidateNum, matrixOnesNum, chrono::duration<double>::zero(),
               chrono::duration<double>::zero(), chrono::duration<double>::zero(), chrono::duration<double>::zero());
        //		VerifyDist(codebook, params.minED, params.maxCodeLen, params.threadNum);
        std::cout << "=====================================================" << std::endl;
        remove("progress_params.txt");
        remove("progress_cand.txt");
    }
    catch (const ifstream::failure &e)
    {
        std::cout << "Read/Write progress files error! Aborted." << std::endl;
    }
}