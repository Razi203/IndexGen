#include "SparseMat.hpp"
#include "Candidates.hpp"
#include "EditDistance.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <climits>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

// *** AdjList Member Function Implementations ***

void AdjList::RowsBySum()
{
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

    return maxSumRow;
}

void AdjList::ToFile(const string &filename) const
{
    ofstream output;
    output.open(filename.c_str());
    if (!output.is_open())
    {
        cout << "Failed opening output file!" << endl;
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
        cout << "Failed opening input file!" << endl;
        return;
    }
    int a, b;
    while (input >> a >> b)
    {
        m[a].insert(b);
    }
    input.close();
}

// *** Standalone Helper Functions ***

void PairsToFile(const string &filename, const vector<pair<int, int>> &vec)
{
    ofstream output;
    output.open(filename.c_str());
    if (!output.is_open())
    {
        cout << "Failed opening output file!" << endl;
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
        cout << "Failed opening input file!" << endl;
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
            cout << "Adj List Comp PROGRESS: i=" << i << " of " << candNum << "\tthreadId\t" << threadIdx << endl;
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
        cout << "Failed opening output file!" << endl;
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
        cout << "Failed opening input file!" << endl;
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
        // // Choose min sum row and delete its ball
        // int minEntry = adjList.FindMinDel(remaining, minSumRowTime, delBallTime);
        // codebook.push_back(candidates[minEntry]);

        // Remove max sum row candidate without adding to codebook
        adjList.FindMaxDel(remaining, minSumRowTime, delBallTime);

        auto currentTime = chrono::steady_clock::now();
        chrono::duration<double> elapsed_seconds = currentTime - lastSaveTime;

        if (saveInterval > 0 && elapsed_seconds.count() > saveInterval)
        {
            SaveProgressCodebook(remaining, adjList, codebook);
            lastSaveTime = currentTime;
            cout << "Codebook PROGRESS: Remaining Rows " << adjList.RowNum() << endl;
        }
    }

    cout << "Find Min Sum Row Time:\t" << fixed << setprecision(2) << minSumRowTime << "\tseconds" << endl;
    cout << "Del Ball Time:\t\t" << fixed << setprecision(2) << delBallTime << "\tseconds" << endl;

    // Once adjList is empty (no edges left), add all remaining vertices to codebook
    for (int num : remaining)
    {
        codebook.push_back(candidates[num]);
    }
    DelProgressCodebook();
}

void CodebookAdjList(const vector<string> &candidates, vector<string> &codebook, const int minED, const int threadNum,
                     const int saveInterval, long long int &matrixOnesNum,
                     std::chrono::duration<double> &fillAdjListTime, std::chrono::duration<double> &processMatrixTime)
{
    AdjList adjList;
    NumToFile(1, "progress_stage.txt");

    auto starta = chrono::steady_clock::now();
    FillAdjList(adjList, candidates, minED, threadNum, saveInterval, false, matrixOnesNum);
    auto enda = chrono::steady_clock::now();
    fillAdjListTime = enda - starta;
    cout << "Fill AdjList Time:\t" << fixed << setprecision(2) << fillAdjListTime.count() << "\tseconds" << endl;

    NumToFile(2, "progress_stage.txt");
    LongLongIntToFile(matrixOnesNum, "matrix_ones_num.txt");

    auto startc = chrono::steady_clock::now();
    Codebook(adjList, codebook, candidates, saveInterval, false);
    auto endc = chrono::steady_clock::now();
    processMatrixTime = endc - startc;
    cout << "Process Matrix Time:\t" << fixed << setprecision(2) << processMatrixTime.count() << "\tseconds" << endl;

    remove("progress_stage.txt");
    remove("matrix_ones_num.txt");
}

void CodebookAdjListResumeFromFile(const vector<string> &candidates, vector<string> &codebook, const Params &params,
                                   long long int &matrixOnesNum)
{
    AdjList adjList;
    int stage;
    FileToNum(stage, "progress_stage.txt");
    if (stage == 1)
    {
        cout << "Resuming adj list comp" << endl;
        FillAdjList(adjList, candidates, params.codeMinED, params.threadNum, params.saveInterval, true, matrixOnesNum);
        NumToFile(2, "progress_stage.txt");
        Codebook(adjList, codebook, candidates, params.saveInterval, false);
    }
    else
    {
        assert(stage == 2);
        cout << "Resuming codebook comp" << endl;
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
    StrVecToFile(candidates, "progress_cand.txt");
    auto end_candidates = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_secs_candidates = end_candidates - start_candidates;
    cout << "Candidates Time: " << fixed << setprecision(2) << elapsed_secs_candidates.count() << "\tseconds" << endl;

    std::chrono::duration<double> fillAdjListTime, processMatrixTime;

    vector<string> codebook;
    long long int matrixOnesNum;

    CodebookAdjList(candidates, codebook, params.codeMinED, params.threadNum, params.saveInterval, matrixOnesNum,
                    fillAdjListTime, processMatrixTime);

    long long int candidateNum = candidates.size();
    PrintTestResults(candidateNum, matrixOnesNum, codebook.size());
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> overAllTime = end - start;
    ToFile(codebook, params, candidateNum, matrixOnesNum, elapsed_secs_candidates, fillAdjListTime, processMatrixTime,
           overAllTime);
    // TODO: Replace with if (flag) do
    VerifyDist(codebook, params.codeMinED, params.threadNum);
    cout << "=====================================================" << endl;
    remove("progress_params.txt");
    remove("progress_cand.txt");

    cout << "Codebook Time: " << fixed << setprecision(2) << overAllTime.count() << "\tseconds" << endl;
    cout << "=====================================================" << endl;
}

void GenerateCodebookAdjResumeFromFile()
{
    try
    {
        Params params;
        FileToParams(params, "progress_params.txt");
        cout << "Resuming Codebook Adj from file" << endl;
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
        cout << "=====================================================" << endl;
        remove("progress_params.txt");
        remove("progress_cand.txt");
    }
    catch (const ifstream::failure &e)
    {
        cout << "Read/Write progress files error! Aborted." << endl;
    }
}
