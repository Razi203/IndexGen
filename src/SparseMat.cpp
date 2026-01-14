#include "SparseMat.hpp"
#include "Candidates.hpp"
#include "EditDistance.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <climits>
#include <cstdlib> // Added for system()
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "clustering/KMeansAdapter.hpp"

namespace {
    /**
     * @brief Get the project root directory by detecting the executable's location.
     * 
     * This function reads /proc/self/exe to find the running executable's path,
     * then navigates up to find the project root (where scripts/ directory exists).
     * Falls back to current directory if detection fails.
     */
    std::string getProjectRoot() {
        namespace fs = std::filesystem;
        
        try {
            // Read the symlink /proc/self/exe to get executable path
            fs::path exe_path = fs::read_symlink("/proc/self/exe");
            fs::path exe_dir = exe_path.parent_path();
            
            // Try to find src/ directory by looking in parent directories
            // Typically: /path/to/project/build/IndexGen -> /path/to/project/src
            fs::path search_path = exe_dir;
            for (int i = 0; i < 5; ++i) { // Look up to 5 levels up
                fs::path src_dir = search_path / "src";
                if (fs::exists(src_dir) && fs::is_directory(src_dir)) {
                    // Verify the GPU script exists
                    if (fs::exists(src_dir / "gpu_graph_generator.py")) {
                        return search_path.string();
                    }
                }
                if (search_path.has_parent_path() && search_path.parent_path() != search_path) {
                    search_path = search_path.parent_path();
                } else {
                    break;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Could not auto-detect project root: " << e.what() << std::endl;
        }
        
        // Fall back to current directory
        return ".";
    }
}


using namespace std;

// *** AdjList Member Function Implementations ***

void AdjList::RowsBySum()
{
    rowsBySum.clear(); // Ensure clear refresh
    for (const pair<const int, unordered_set<int>> &iSetJ : m)
    {
        int i = iSetJ.first;
        int sumRowI = iSetJ.second.size();
        rowsBySum[sumRowI].insert(i);
    }
}

int AdjList::MinSumRow() const
{
    assert(not rowsBySum.empty());
    const unordered_set<int> &minSumSet = rowsBySum.begin()->second;
    assert(not minSumSet.empty());
    return *(minSumSet.begin());
}

int AdjList::MaxSumRow() const
{
    assert(not rowsBySum.empty());
    const unordered_set<int> &maxSumSet = rowsBySum.rbegin()->second;
    assert(not maxSumSet.empty());
    return *(maxSumSet.begin());
}

void AdjList::DeleteRow(const int currentSum, const int row)
{
    map<int, unordered_set<int>>::iterator currSumIt = rowsBySum.find(currentSum);
    assert(currSumIt != rowsBySum.end());
    int erasedNum = currSumIt->second.erase(row);
    assert(erasedNum == 1);
    if (currSumIt->second.empty())
    {
        rowsBySum.erase(currSumIt);
    }
}

void AdjList::DecreaseSum(const int currentSum, const int row)
{
    assert(currentSum > 0);
    DeleteRow(currentSum, row);
    rowsBySum[currentSum - 1].insert(row);
}

// Helper function to remove empty rows (no edges left)
// @return number of rows removed
// TODO: check if implemented correctly (should be)
int AdjList::RemoveEmptyRows()
{
    int removedRowsNum = 0;
    map<int, unordered_set<int>>::iterator rowIt = rowsBySum.find(0);
    if (rowIt != rowsBySum.end())
    {
        removedRowsNum = rowIt->second.size();
        for (int row : rowIt->second)
        {
            m.erase(row);
        }
        rowsBySum.erase(rowIt);
    }
    return removedRowsNum;
}

bool AdjList::empty() const
{
    return m.empty();
}

int AdjList::RowNum() const
{
    return m.size();
}

void AdjList::Set(int row, int col)
{
    m[row].insert(col);
}

void AdjList::DelRowCol(int rc)
{
    unordered_map<int, unordered_set<int>>::iterator rowIt = m.find(rc);
    int i = rowIt->first;
    unordered_set<int> &js = rowIt->second;
    // delete col for each j in js delete (j,i)
    for (int j : js)
    {
        unordered_map<int, unordered_set<int>>::iterator jRowIt = m.find(j);
        assert(jRowIt != m.end());
        int currRowJSum = jRowIt->second.size();
        int eraseNum = jRowIt->second.erase(i);
        assert(eraseNum == 1);
        // decrease sum of row j
        DecreaseSum(currRowJSum, j);
    }
    // delete row
    DeleteRow(js.size(), i);
    m.erase(rowIt);
}

void AdjList::DelBall(const int matRow, unordered_set<int> &remaining)
{
    unordered_map<int, unordered_set<int>>::iterator rowIt = m.find(matRow);
    assert(rowIt != m.end());
    vector<int> toDel(rowIt->second.begin(), rowIt->second.end());
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
    for (const pair<const int, unordered_set<int>> &numSetPr : m)
    {
        int i = numSetPr.first;
        for (int j : numSetPr.second)
        {
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
    while (input >> a >> b)
    {
        m[a].insert(b);
    }
    input.close();
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
        m[u].insert(v);
        m[v].insert(u);
    }
    matrixOnesNum = numInts;
    if(!silent) std::cout << "Loaded " << (numInts / 2) << " edges from binary file." << endl;
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
                    const string &inputFilename = "", double maxGPUMemoryGB = 10.0, bool silent = false)
{
    string vecFile = "temp_vectors.txt";
    string edgesFile = "temp_edges.bin";

    string fileToUse = vecFile;

    // OPTIMIZATION: If we already have a file on disk (progress_cand.txt), use it!
    if (!inputFilename.empty())
    {
        if(!silent) std::cout << "[C++] Using existing candidate file: " << inputFilename << endl;
        fileToUse = inputFilename;
    }
    else
    {
        // Fallback: Save candidates to temp file
        if(!silent) std::cout << "[C++] Saving candidates to " << vecFile << "..." << endl;
        StrVecToFile(candidates, vecFile);
    }

    // 2. Call Python Script
    // Usage: python gpu_graph_generator.py <input> <output> <threshold>
    // Determine script path: auto-detect from executable location, allow env override
    const char* env_root = std::getenv("INDEXGEN_ROOT");
    string project_root = (env_root) ? string(env_root) : getProjectRoot();
    string script_path = project_root + "/src/gpu_graph_generator.py";

    string cmd = "python " + script_path + " " + fileToUse + " " +
                 edgesFile + " " + to_string(minED) + " " + to_string(maxGPUMemoryGB);
    
    if (silent) {
        cmd += " > /dev/null 2>&1";
    } else {
        std::cout << "[C++] Executing GPU Solver: " << cmd << endl;
    }

    int ret = system(cmd.c_str());
    if (ret != 0)
    {
        if (silent) {
            // If it failed silently, try to run again without silence to show error, or just print generic error
             std::cerr << "Error: Python GPU script failed (run without silence to see details)." << endl;
        } else {
            std::cerr << "Error: Python GPU script failed." << endl;
        }
        exit(1);
    }

    // 3. Load Binary Edges
    if(!silent) std::cout << "[C++] Loading edges from " << edgesFile << "..." << endl;
    auto load_start = chrono::steady_clock::now();

    adjList.FromBinaryFile(edgesFile, matrixOnesNum, silent);

    auto load_end = chrono::steady_clock::now();
    chrono::duration<double> load_time = load_end - load_start;
    if(!silent) std::cout << "[C++] Edge load time: " << fixed << setprecision(2) << load_time.count() << "s" << endl;

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

// Updated signature to include useGPU flag
void CodebookAdjList(const vector<string> &candidates, vector<string> &codebook, const int minED, const int threadNum,
                     const int saveInterval, long long int &matrixOnesNum,
                     std::chrono::duration<double> &fillAdjListTime, std::chrono::duration<double> &processMatrixTime,
                     const bool useGPU, double maxGPUMemoryGB, const string &candFilename = "") // ADDED Argument
{
    AdjList adjList;
    NumToFile(1, "progress_stage.txt");

    auto starta = chrono::steady_clock::now();

    if (useGPU)
    {
        // --- INTEGRATION CHANGE: USE GPU SOLVER ---
        // Pass the existing filename (candFilename) to avoid redundant writing
        std::cout << "[C++] Mode: GPU Accelerated (Max Mem: " << maxGPUMemoryGB << " GB)" << endl;
        FillAdjListGPU(adjList, candidates, minED, matrixOnesNum, candFilename, maxGPUMemoryGB);
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
                                            const int threadNum, const bool useGPU, double maxGPUMemoryGB)
{
    // If empty or trivial
    if (candidates.empty()) return {};
    if (candidates.size() == 1) return candidates;

    AdjList adjList;
    long long int matrixOnesNum = 0;
    
    // Fill AdjList (Graph Construction)
    // We use a temporary filename for GPU interaction if needed
    // Use a unique name to avoid collision if running in parallel logic (though currently sequential)
    static int call_count = 0;
    string candFilename = "temp_cand_" + to_string(call_count++) + ".txt";
    
    // We need to write candidates to file for GPU logic
    if (useGPU) {
        StrVecToFile(candidates, candFilename);
    }

    if (useGPU)
    {
         // Suppress some output during inner loops
        // std::cout << "..." << endl; 
        FillAdjListGPU(adjList, candidates, minED, matrixOnesNum, candFilename, maxGPUMemoryGB, true);
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
    
    double d1=0, d2=0;
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
    if (useGPU) {
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
    
    if (!params.clustering.enabled)
    {
        // OLD BEHAVIOR: One pass
        std::cout << "Clustering disabled. Running standard generation..." << std::endl;
        // Pass candFilename to the function
        CodebookAdjList(candidates, codebook, params.codeMinED, params.threadNum, params.saveInterval, matrixOnesNum,
                        fillAdjListTime, processMatrixTime, params.useGPU, params.maxGPUMemoryGB, candFilename);
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
            std::cout << "Starting Iteration " << iteration << ": Candidate set size = " << NumberWithCommas(current_candidates.size()) << std::endl;
            
            // Step 1: Cluster
            auto cluster_start = std::chrono::steady_clock::now();
            
            // Check trivial case
            if (current_candidates.empty()) break;
            
            indexgen::clustering::KMeansAdapter adapter(params.clustering.k);
            auto clusters = adapter.cluster(current_candidates);
            
            auto cluster_end = std::chrono::steady_clock::now();
            double clustering_time = std::chrono::duration<double>(cluster_end - cluster_start).count();

            // Step 2: Solve Independent Set per Cluster
            std::vector<std::string> next_candidates;
            std::vector<std::vector<std::string>> cluster_results(clusters.size());
            
            auto solve_start = std::chrono::steady_clock::now();
            
            // We'll collect individual times for average
            std::vector<double> individual_solve_times;
            individual_solve_times.reserve(clusters.size());

            for(int i=0; i< (int)clusters.size(); ++i) {
                auto single_solve_start = std::chrono::steady_clock::now();
                cluster_results[i] = SolveIndependentSet(clusters[i], params.codeMinED, params.threadNum, params.useGPU, params.maxGPUMemoryGB);
                auto single_solve_end = std::chrono::steady_clock::now();
                individual_solve_times.push_back(std::chrono::duration<double>(single_solve_end - single_solve_start).count());
            }
            
            auto solve_end = std::chrono::steady_clock::now();
            double solving_total_time = std::chrono::duration<double>(solve_end - solve_start).count();

            // Step 3: Combine
            for(const auto& res : cluster_results) {
                next_candidates.insert(next_candidates.end(), res.begin(), res.end());
            }
            
            // Step 4: Check Convergence (3 iterations identical size)
            previous_sizes.push_back(next_candidates.size());
            
            auto iter_end = std::chrono::steady_clock::now();
            double iteration_total_time = std::chrono::duration<double>(iter_end - iter_start).count();

            // VERBOSE OUTPUT
            if (params.clustering.verbose) {
                double avg_solve_time = 0.0;
                if (!individual_solve_times.empty()) {
                    double sum = 0;
                    for(auto t : individual_solve_times) sum += t;
                    avg_solve_time = sum / individual_solve_times.size();
                }
                
                std::cout << "[Verbose] Iteration " << iteration << " Stats:" << std::endl;
                std::cout << "  - Total Iteration Time: " << fixed << setprecision(3) << iteration_total_time << " s" << std::endl;
                std::cout << "  - Clustering Time:      " << fixed << setprecision(3) << clustering_time << " s" << std::endl;
                std::cout << "  - Total Solving Time:   " << fixed << setprecision(3) << solving_total_time << " s" << std::endl;
                std::cout << "  - Avg Cluster Solve:    " << fixed << setprecision(3) << avg_solve_time << " s" << std::endl;
            }

            if (previous_sizes.size() >= 3) {
                size_t n = previous_sizes.size();
                if (previous_sizes[n-1] == previous_sizes[n-2] && previous_sizes[n-2] == previous_sizes[n-3]) {
                    std::cout << "Convergence reached! Sizes: " << previous_sizes[n-3] << " -> " << previous_sizes[n-2] << " -> " << previous_sizes[n-1] << std::endl;
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
    if(params.clustering.enabled) {
        matrixOnesNum = 0;
        clusterK = params.clustering.k;
        clusterIterations = final_iteration;
    }
    
    PrintTestResults(candidateNum, matrixOnesNum, codebook.size(), clusterK, clusterIterations);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> overAllTime = end - start;
    
    // Note: Timing stats for "FillAdjListTime" etc are not aggregated in the iterative loop. 
    // Pass zero or implement aggregation if needed.
    ToFile(codebook, params, candidateNum, matrixOnesNum, elapsed_secs_candidates, fillAdjListTime, processMatrixTime,
           overAllTime, clusterK, clusterIterations);
    if (params.verify)
    {
        VerifyDist(codebook, params.codeMinED, params.threadNum);
    }
    std::cout << "=====================================================" << std::endl;
    remove("progress_params.txt");
    remove("progress_cand.txt");

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
        PrintTestResults(candidateNum, matrixOnesNum, codebook.size());
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