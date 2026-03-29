#include "Utils.hpp"
#include "CandidateGenerator.hpp"
#include "Candidates/LinearCodes.hpp"
#include "EditDistance.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <random>
#include <thread>

int FastEditDistance(const string &source, const string &target)
{
    if (source.size() > target.size())
    {
        return FastEditDistance(target, source);
    }

    const int min_size = source.size(), max_size = target.size();
    vector<int> lev_dist(min_size + 1);

    for (int i = 0; i <= min_size; ++i)
    {
        lev_dist[i] = i;
    }

    for (int j = 1; j <= max_size; ++j)
    {
        int previous_diagonal = lev_dist[0], previous_diagonal_save;
        ++lev_dist[0];

        for (int i = 1; i <= min_size; ++i)
        {
            previous_diagonal_save = lev_dist[i];
            if (source[i - 1] == target[j - 1])
            {
                lev_dist[i] = previous_diagonal;
            }
            else
            {
                lev_dist[i] = min(min(lev_dist[i - 1], lev_dist[i]), previous_diagonal) + 1;
            }
            previous_diagonal = previous_diagonal_save;
        }
    }

    return lev_dist[min_size];
}
// if ED(source,target) >= minED return true. otherwise, false.
bool FastEditDistance(const string &source, const string &target, const int minED)
{
    if (source.size() > target.size())
    {
        return FastEditDistance(target, source, minED);
    }

    const int min_size = source.size(), max_size = target.size();
    vector<int> lev_dist(min_size + 1);

    for (int i = 0; i <= min_size; ++i)
    {
        lev_dist[i] = i;
    }

    for (int j = 1; j <= max_size; ++j)
    {
        int previous_diagonal = lev_dist[0], previous_diagonal_save;
        ++lev_dist[0];

        for (int i = 1; i <= min_size; ++i)
        {
            previous_diagonal_save = lev_dist[i];
            if (source[i - 1] == target[j - 1])
            {
                lev_dist[i] = previous_diagonal;
            }
            else
            {
                lev_dist[i] = min(min(lev_dist[i - 1], lev_dist[i]), previous_diagonal) + 1;
            }
            previous_diagonal = previous_diagonal_save;
        }
        if (lev_dist[j] >= minED)
        {
            return true;
        }
    }

    return lev_dist[min_size] >= minED;
}

int SumAbs0123Diff(const vector<char> &acgtContx, const vector<char> &acgtConty)
{
    int sum = 0;
    for (int i = 0; i < 4; i++)
    {
        sum += abs(acgtContx[i] - acgtConty[i]);
    }
    return sum;
}

bool FastEditDistance0123(const string &X, const string &Y, const int minED, const vector<char> &contx,
                          const vector<char> &conty)
{
    int sumAbsACGTDiff = SumAbs0123Diff(contx, conty);
    if (sumAbsACGTDiff / 2 >= minED)
        return true;
    else
        return FastEditDistance(X, Y, minED);
}

int FastEditDistanceForSearch(const string &source, const string &target, const int minED)
{
    if (source.size() > target.size())
    {
        return FastEditDistance(target, source, minED);
    }

    const int min_size = source.size(), max_size = target.size();
    vector<int> lev_dist(min_size + 1);

    for (int i = 0; i <= min_size; ++i)
    {
        lev_dist[i] = i;
    }

    for (int j = 1; j <= max_size; ++j)
    {
        int previous_diagonal = lev_dist[0], previous_diagonal_save;
        ++lev_dist[0];

        for (int i = 1; i <= min_size; ++i)
        {
            previous_diagonal_save = lev_dist[i];
            if (source[i - 1] == target[j - 1])
            {
                lev_dist[i] = previous_diagonal;
            }
            else
            {
                lev_dist[i] = min(min(lev_dist[i - 1], lev_dist[i]), previous_diagonal) + 1;
            }
            previous_diagonal = previous_diagonal_save;
        }
        if (lev_dist[j] >= minED)
        {
            return -1;
        }
    }

    if (lev_dist[min_size] >= minED)
        return -1;
    else
        return lev_dist[min_size];
}

// if ED(X,Y)>= min ED return -1. Otherwise, return ED(X,Y)

int FastEditDistance0123ForSearch(const string &X, const string &Y, const int minED, const vector<char> &contx,
                                  const vector<char> &conty)
{
    int sumAbsACGTDiff = SumAbs0123Diff(contx, conty);
    if (sumAbsACGTDiff / 2 >= minED) // TODO: how to use minED-1
        return -1;
    else
        return FastEditDistanceForSearch(X, Y, minED);
}

int HammingDist(const string &str1, const string &str2)
{
    assert(str1.size() == str2.size());
    int count = 0;
    for (int i = 0; i < (int)str1.size(); i++)
    {
        if (str1[i] != str2[i])
            count++;
    }
    return count;
}

void VerifyDistT(const vector<string> &vecs, const int minED, const int threadIdx, const int threadNum,
                 atomic<bool> &success)
{
    for (unsigned i = threadIdx; i < vecs.size(); i += threadNum)
    {
        PatternHandle handle = MakePattern(vecs[i]);
        for (unsigned j = i + 1; j < vecs.size(); j++)
        {
            // int ED = FastEditDistance(vecs[i], vecs[j]); // Old version
            int ED = EditDistanceBanded(vecs[j], handle, minED - 1);
            if (ED < minED)
            {
                success = false;
                return;
            }
        }
    }
    success = true;
}

void VerifyDist(vector<string> &vecs, const int minED, const int threadNum, bool useGPU, double maxGPUMemoryGB, bool isBinary)
{
    if (useGPU)
    {
        // GPU-accelerated verification using gpu_graph_generator.py
        // The script finds all pairs with edit distance < threshold.
        // If 0 edges are found, the codebook is valid.
        namespace fs = std::filesystem;
        
        string vecFile = "temp_verify_vectors.txt";
        string edgesFile = "temp_verify_edges.bin";
        
        // Write codebook to temp file
        StrVecToFile(vecs, vecFile);
        
        // Detect project root
        std::string project_root = ".";
        try {
            fs::path exe_path = fs::read_symlink("/proc/self/exe");
            fs::path search_path = exe_path.parent_path();
            for (int i = 0; i < 5; ++i) {
                if (fs::exists(search_path / "src" / "gpu_graph_generator.py")) {
                    project_root = search_path.string();
                    break;
                }
                if (search_path.has_parent_path() && search_path.parent_path() != search_path)
                    search_path = search_path.parent_path();
                else
                    break;
            }
        } catch (...) {}
        
        const char *env_root = std::getenv("INDEXGEN_ROOT");
        if (env_root) project_root = string(env_root);
        
        string script_path = project_root + (isBinary ? "/src/gpu_graph_generator_binary.py" : "/src/gpu_graph_generator.py");
        string cmd = "python3 " + script_path + " " + vecFile + " " + edgesFile + " " +
                     to_string(minED) + " " + to_string(maxGPUMemoryGB);
        
        cout << "[Verify GPU] Running: " << cmd << endl;
        auto verify_start = chrono::steady_clock::now();
        
        int ret = system(cmd.c_str());
        if (ret != 0)
        {
            cerr << "[Verify GPU] Error: GPU script failed." << endl;
            remove(vecFile.c_str());
            remove(edgesFile.c_str());
            return;
        }
        
        // Read edge count from binary file
        long long edgeCount = 0;
        {
            ifstream input(edgesFile, ios::binary);
            if (input.is_open())
            {
                input.seekg(0, ios::end);
                size_t fileSize = input.tellg();
                // Each edge is 2 int32_t values = 8 bytes
                edgeCount = fileSize / (2 * sizeof(int32_t));
                input.close();
            }
        }
        
        auto verify_end = chrono::steady_clock::now();
        double verify_time = chrono::duration<double>(verify_end - verify_start).count();
        
        cout << "[Verify GPU] Edges found (violations): " << edgeCount << endl;
        cout << "[Verify GPU] Time: " << fixed << setprecision(2) << verify_time << " s" << endl;
        
        if (edgeCount == 0)
        {
            cout << "Vector distance SUCCESS" << endl;
        }
        else
        {
            cout << "Vector distance FAILURE" << endl;
        }
        
        remove(vecFile.c_str());
        remove(edgesFile.c_str());
    }
    else
    {
        // CPU verification (original path)
        atomic<bool> success(false);
        vector<thread> threads;
        for (int i = 0; i < threadNum; i++)
        {
            threads.push_back(thread(VerifyDistT, vecs, minED, i, threadNum, ref(success)));
        }
        for (thread &th : threads)
            th.join();
        if (success)
        {
            cout << "Vector distance SUCCESS" << endl;
        }
        else
        {
            cout << "Vector distance FAILURE" << endl;
        }
    }
}

void PrintParamsToFile(std::ofstream &out, const int candidateNum, const int codeSize, const Params &params,
                       const long long int matrixOnesNum, const std::chrono::duration<double> &candidatesTime,
                       const std::chrono::duration<double> &fillAdjListTime,
                       const std::chrono::duration<double> &processMatrixTime,
                       const std::chrono::duration<double> &overallTime,
                       int clusterK = -1, int clusterIterations = -1)
{

    // Using std::endl for consistency
    out << "--- Global Parameters ---" << std::endl;
    out << "Code Length:\t\t\t" << params.codeLen << std::endl;
    out << "Min Codebook Edit Distance:\t" << params.codeMinED << std::endl;

    out << std::endl;

    out << "Max Run:\t\t\t" << params.maxRun << std::endl;
    out << "Min GC Content:\t\t\t" << params.minGCCont << std::endl;
    out << "Max GC Content:\t\t\t" << params.maxGCCont << std::endl;

    out << std::endl;

    // --- Generation Method & Specifics ---
    try
    {
        std::shared_ptr<CandidateGenerator> generator = CreateGenerator(params);
        generator->printInfo(out);
    }
    catch (const std::exception &e)
    {
        out << "Error creating generator: " << e.what() << endl;
    }

    out << std::endl;
    out << "--- Results Summary ---" << std::endl;
    out << "Number of Candidates:\t\t" << candidateNum << std::endl;
    if (clusterK > 0) {
        out << "Clustering Method:\t\t" << params.clustering.method << std::endl;
        out << "Number of Clusters (K):\t\t" << clusterK << std::endl;
        out << "Required Identical Iterations:\t" << params.clustering.convergenceIterations << std::endl;
        out << "Iterations to Converge:\t\t" << clusterIterations << std::endl;
    } else {
        out << "Number of Ones in Matrix:\t" << matrixOnesNum << std::endl;
    }
    out << "Number of Code Words:\t\t" << codeSize << std::endl;

    out << std::endl;
    out << "--- Performance Metrics ---" << std::endl;
    out << "Number of Threads:\t\t" << params.threadNum << std::endl;
    out << "Candidate Generation Time:\t" << fixed << setprecision(2) << candidatesTime.count() << "\tseconds"
        << std::endl;
    if (clusterK > 0 && clusterIterations > 0) {
        out << "Total Clustering Time:\t\t" << fixed << setprecision(2) << fillAdjListTime.count() << "\tseconds"
            << std::endl;
        out << "Avg Clustering Time/Iter:\t" << fixed << setprecision(2) << fillAdjListTime.count() / clusterIterations << "\tseconds"
            << std::endl;
        out << "Total Solving Time:\t\t" << fixed << setprecision(2) << processMatrixTime.count() << "\tseconds"
            << std::endl;
        out << "Avg Solving Time/Iter:\t\t" << fixed << setprecision(2) << processMatrixTime.count() / clusterIterations << "\tseconds"
            << std::endl;
    } else {
        out << "Fill Adjacency List Time:\t" << fixed << setprecision(2) << fillAdjListTime.count() << "\tseconds"
            << std::endl;
        out << "Process Matrix Time:\t\t" << fixed << setprecision(2) << processMatrixTime.count() << "\tseconds"
            << std::endl;
    }
    out << "Overall Execution Time:\t\t" << fixed << setprecision(2) << overallTime.count() << "\tseconds" << std::endl;

    out << "=========================================== " << std::endl;
}

string FileName(const int codeLen, const int codeSize, const int minED)
{
    string codeLenStr = to_string(codeLen);
    assert(codeLenStr.size() <= 2);
    codeLenStr = string(2 - codeLenStr.size(), '0') + codeLenStr;
    string codeSizeStr = to_string(codeSize);
    assert(codeSizeStr.size() <= 7);
    codeSizeStr = string(7 - codeSizeStr.size(), '0') + codeSizeStr;
    string minEDStr = to_string(minED);
    string fileName = "CodeSize-" + codeSizeStr + '_' + "CodeLen-" + codeLenStr + '_' + "MinED-" + minEDStr + ".txt";
    return fileName;
}

void ToFile(const vector<string> &codeWords, const Params &params, const int candidateNum,
            const long long int matrixOnesNum, const std::chrono::duration<double> &candidatesTime,
            const std::chrono::duration<double> &fillAdjListTime,
            const std::chrono::duration<double> &processMatrixTime, const std::chrono::duration<double> &overallTime,
            int clusterK, int clusterIterations)
{
    int codeSize = codeWords.size();
    ofstream output;
    string fileName = FileName(params.codeLen, codeSize, params.codeMinED);
    output.open(fileName.c_str());
    if (!output.is_open())
    {
        cout << "Failed opening output file!" << endl;
        return;
    }

    PrintParamsToFile(output, candidateNum, codeSize, params, matrixOnesNum, candidatesTime, fillAdjListTime,
                      processMatrixTime, overallTime, clusterK, clusterIterations);

    for (const string &word : codeWords)
    {
        output << word << '\n';
    }
    output.close();
}

int MaxRun(const string &str)
{
    int start = 0;
    char runLetter = str[0];
    int maxRun = 0;
    for (int i = 1; i < (int)str.size(); i++)
    {
        if (str[i] == runLetter)
        {
        }
        else
        { // run ended
            int runLen = i - start;
            if (runLen > maxRun)
                maxRun = runLen;
            start = i;
            runLetter = str[i];
        }
    }
    int runLen = (int)str.size() - start;
    if (runLen > maxRun)
        maxRun = runLen;

    return maxRun;
}

// return fraction of number of occurrences of '1'(C) and '2'(G)
double GCContent(const string &a)
{
    int GCNum = 0;
    for (char letter : a)
    {
        if (letter == '1' or letter == '2')
            GCNum++;
    }
    return double(GCNum) / double(a.size());
}

bool TestGCCont(const string &a, const double minGCCont, const double maxGCCont)
{
    double GCCont = GCContent(a);
    return (GCCont >= minGCCont) && (GCCont <= maxGCCont);
}

double BinaryOneContent(const string &a)
{
    int oneCount = 0;
    for (char letter : a)
    {
        if (letter == '1')
            oneCount++;
    }
    return double(oneCount) / double(a.size());
}

bool TestBinaryContent(const string &a, const double minCont, const double maxCont)
{
    double cont = BinaryOneContent(a);
    return (cont >= minCont) && (cont <= maxCont);
}

bool TestAllLettersOccurence(const string &a)
{
    vector<int> occs(4);
    for (char letter : a)
    {
        occs[letter - '0']++;
    }
    for (int occ : occs)
    {
        if (occ == 0)
        {
            return false;
        }
    }
    return true;
}

// str - string with a number in base 4
// return string of next binary number
// if str is highest number with len of string return empty string
string NextBase4(const string &str)
{
    string result = str;
    for (string::reverse_iterator it = result.rbegin(); it != result.rend(); it++)
    {
        char currLetter = *it;
        if (currLetter < '3')
        {
            *it = currLetter + 1;
            return result;
        }
        else
        {
            *it = '0';
        }
    }
    result.clear();
    return result;
}

// vec - vector of ints over {0, 1, 2, 3} representing a base 4 number
// return next base 4 number
// if number is highest possible with give number of digits return empty vector

vector<int> NextBase4(const vector<int> &vec)
{
    vector<int> result = vec;
    for (vector<int>::reverse_iterator it = result.rbegin(); it != result.rend(); it++)
    {
        int currDigit = *it;
        if (currDigit < 3)
        {
            (*it)++;
            return result;
        }
        else
        {
            *it = 0;
        }
    }
    result.clear();
    return result;
}

vector<int> NextBase2(const vector<int> &vec)
{
    vector<int> result = vec;
    for (vector<int>::reverse_iterator it = result.rbegin(); it != result.rend(); it++)
    {
        int currDigit = *it;
        if (currDigit < 1)
        {
            (*it)++;
            return result;
        }
        else
        {
            *it = 0;
        }
    }
    result.clear();
    return result;
}
// for w root of x^2+x+1, w=2, w^2=3

int AddF4(const int a, const int b)
{
    assert((0 <= a) && (a < 4) && (0 <= b) && (b < 4));
    static const int addition[4][4] = {{0, 1, 2, 3}, {1, 0, 3, 2}, {2, 3, 0, 1}, {3, 2, 1, 0}};
    return addition[a][b];
}

int MulF4(const int a, const int b)
{
    assert((0 <= a) && (a < 4) && (0 <= b) && (b < 4));
    static const int mul[4][4] = {{0, 0, 0, 0}, {0, 1, 2, 3}, {0, 2, 3, 1}, {0, 3, 1, 2}};
    return mul[a][b];
}

// GF4 multiplication v(1xk) M(kxl)
vector<int> MatMul(const vector<int> &v, const vector<vector<int>> &M, const int k, const int l)
{
    assert(not M.empty());
    assert((int)M.size() == k);
    assert((int)M[0].size() == l);
    vector<int> result(l);
    for (int j = 0; j < l; j++)
    {
        int res = 0;
        for (int i = 0; i < k; i++)
        {
            res = AddF4(res, MulF4(v[i], M[i][j]));
        }
        result[j] = res;
    }
    return result;
}

// vec - vector of ints from { 0, 1, 2, 3}
// return string of {'0','1', '2', '3'}
string VecToStr(const vector<int> &vec)
{
    string result;
    for (int num : vec)
    {
        result += num + '0';
    }
    return result;
}

void VerifyHammDist(const vector<string> &vecs, const int minHammDist)
{
    for (unsigned i = 0; i < vecs.size(); i++)
    {
        for (unsigned j = i + 1; j < vecs.size(); j++)
        {
            int dist = HammingDist(vecs[i], vecs[j]);
            if (dist < minHammDist)
            {
                cout << vecs[i] << endl;
                cout << vecs[j] << endl;
                cout << "Vector distance FAILURE" << endl;
                return;
            }
        }
    }
    cout << "Vector distance SUCCESS" << endl;
}

bool IsZeroVec(const vector<int> &vec)
{
    for (int num : vec)
    {
        if (num != 0)
        {
            return false;
        }
    }
    return true;
}

void FindIndexAndLambda(const vector<int> &parityVec, const vector<vector<int>> &H, int &index, int &lambda)
{
    for (int lam = 1; lam < 4; lam++)
    {
        for (int i = 0; i < (int)H.size(); i++)
        {
            vector<int> lambdaHi;
            for (int num : H[i])
            {
                lambdaHi.push_back(MulF4(num, lam));
            }
            if (lambdaHi == parityVec)
            {
                index = i;
                lambda = lam;
                return;
            }
        }
    }
}

vector<int> CorrectCodeVec(const vector<int> &codeVec, const vector<vector<int>> &H, int maxCodeLen, int redundancy)
{
    int codeLen = codeVec.size();
    vector<vector<int>> TrimmedH(H.begin() + (maxCodeLen - codeLen), H.end());
    vector<int> parityVec = MatMul(codeVec, TrimmedH, codeLen, redundancy);
    if (IsZeroVec(parityVec))
    {
        return codeVec;
    }
    else
    {
        int index = 0, lambda = 0;
        FindIndexAndLambda(parityVec, TrimmedH, index, lambda);
        vector<int> result = codeVec;
        result[index] = AddF4(result[index], lambda);
        return result;
    }
}

void TestDecode(const int codeLen, const vector<vector<int>> &H, int maxCodeLen, int redundancy)
{
    unsigned sd = chrono::high_resolution_clock::now().time_since_epoch().count();
    mt19937 generator(sd);
    vector<vector<int>> codeVecs = CodedVecs(codeLen, 3);
    cout << "Code vecs num:\t" << codeVecs.size() << endl;
    for (const vector<int> &codeVec : codeVecs)
    {
        vector<int> receivedVec = codeVec;
        uniform_int_distribution<int> idxDist(0, codeLen - 1);
        int errorIdx = idxDist(generator);
        uniform_int_distribution<int> numDist(0, 3);
        int errorNum = numDist(generator);
        receivedVec[errorIdx] = errorNum;
        vector<int> correctedVec = CorrectCodeVec(receivedVec, H, maxCodeLen, redundancy);
        if (correctedVec != codeVec)
        {
            cout << "Decode Test FAILURE\n";
            return;
        }
    }
    cout << "Decode Test SUCCESS\n";
}

void PrintTestParams(const Params &params)
{
    using std::cout;
    using std::endl;

    // --- Print Common Parameters ---
    cout << "--- Common Parameters ---" << endl;
    cout << "Code Length:\t\t\t" << params.codeLen << endl;
    cout << "Min Codebook Edit Distance:\t" << params.codeMinED << endl;
    cout << "Max Homopolymer Run:\t\t" << params.maxRun << endl;
    cout << "Min GC Content:\t\t\t" << params.minGCCont << endl;
    cout << "Max GC Content:\t\t\t" << params.maxGCCont << endl;
    cout << endl;
    // --- Use the generator's printInfo() method ---
    try
    {
        std::shared_ptr<CandidateGenerator> generator = CreateGenerator(params);
        generator->printInfo(cout);
    }
    catch (const std::exception &e)
    {
        cout << "Error creating generator: " << e.what() << endl;
    }
    cout << endl;
}

void PrintTestResults(const int candidateNum, const long long int matrixOnesNum, const int codewordsNum,
                      int clusterK, int clusterIterations, int requiredIterations,
                      const std::string &clusteringMethod)
{
    if (clusterK > 0) {
        if (!clusteringMethod.empty()) {
            cout << "Clustering Method:\t\t" << clusteringMethod << endl;
        }
        cout << "Number of Clusters (K):\t\t" << clusterK << endl;
        cout << "Required Identical Iterations:\t" << requiredIterations << endl;
        cout << "Iterations to Converge:\t\t" << clusterIterations << endl;
    } else {
        cout << "Number of Ones in Matrix:\t" << matrixOnesNum << endl;
    }
    cout << "Number of Code Words:\t\t" << codewordsNum << endl;
}

void ParamsToFile(const Params &params, const std::string &fileName)
{
    std::ofstream output_file;
    output_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    output_file.open(fileName);

    // 1. Write all common parameters from the main Params struct
    output_file << params.codeLen << '\n';
    output_file << params.codeMinED << '\n';
    output_file << params.maxRun << '\n';
    output_file << params.minGCCont << '\n';
    output_file << params.maxGCCont << '\n';
    output_file << params.threadNum << '\n';
    output_file << params.saveInterval << '\n';
    output_file << params.verify << '\n';
    output_file << params.useGPU << '\n';
    output_file << params.maxGPUMemoryGB << '\n';
    output_file << params.clustering.enabled << '\n';
    output_file << params.clustering.k << '\n';
    output_file << params.clustering.verbose << '\n';
    output_file << params.clustering.convergenceIterations << '\n';
    output_file << params.clustering.method << '\n';

    // 2. Write the generation method type identifier (as an integer)
    output_file << static_cast<int>(params.method) << '\n';

    // 3. Write the method-specific parameters using the generator's printParams method
    try
    {
        std::shared_ptr<CandidateGenerator> generator = CreateGenerator(params);
        generator->printParams(output_file);
    }
    catch (const std::exception &e)
    {
        output_file.close();
        throw std::runtime_error(std::string("Error writing parameters: ") + e.what());
    }

    output_file.close();
}

void FileToParams(Params &params, const std::string &fileName)
{
    std::ifstream input_file;
    input_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    input_file.open(fileName);

    // 1. Read all common parameters
    input_file >> params.codeLen;
    input_file >> params.codeMinED;
    input_file >> params.maxRun;
    input_file >> params.minGCCont;
    input_file >> params.maxGCCont;
    input_file >> params.threadNum;
    input_file >> params.saveInterval;
    input_file >> params.verify;
    input_file >> params.useGPU;
    input_file >> params.maxGPUMemoryGB;
    input_file >> params.clustering.enabled;
    input_file >> params.clustering.k;
    input_file >> params.clustering.verbose;
    input_file >> params.clustering.convergenceIterations;
    input_file >> params.clustering.method;

    // 2. Read the integer, then cast it back to the GenerationMethod enum
    int method_type_int;
    input_file >> method_type_int;
    params.method = static_cast<GenerationMethod>(method_type_int);

    // 3. Create default constraints based on method type, then use generator's readParams
    try
    {
        // First create empty constraints based on the method type
        switch (params.method)
        {
        case GenerationMethod::LINEAR_CODE:
            params.constraints = std::make_unique<LinearCodeConstraints>();
            break;
        case GenerationMethod::VT_CODE:
            params.constraints = std::make_unique<VTCodeConstraints>();
            break;
        case GenerationMethod::RANDOM:
            params.constraints = std::make_unique<RandomConstraints>();
            break;
        case GenerationMethod::ALL_STRINGS:
            params.constraints = std::make_unique<AllStringsConstraints>();
            break;
        case GenerationMethod::DIFFERENTIAL_VT_CODE:
            params.constraints = std::make_unique<DifferentialVTCodeConstraints>();
            break;
        case GenerationMethod::FILE_READ:
            params.constraints = std::make_unique<FileReadConstraints>();
            break;
        case GenerationMethod::LINEAR_BINARY_CODE:
            params.constraints = std::make_unique<LinearBinaryCodeConstraints>();
            break;
        default:
            throw std::runtime_error("Unknown generation method type found in file.");
        }

        // Create generator and use its readParams method to populate constraints
        std::shared_ptr<CandidateGenerator> generator = CreateGenerator(params);
        generator->readParams(input_file, params.constraints.get());
    }
    catch (const std::exception &e)
    {
        input_file.close();
        throw std::runtime_error(std::string("Error reading parameters: ") + e.what());
    }

    input_file.close();
}

void IntVecToFile(const vector<int> &data, const string &fileName)
{
    ofstream output_file;
    output_file.exceptions(ofstream::failbit | ofstream::badbit);
    output_file.open(fileName);

    ostream_iterator<int> output_iterator(output_file, "\n");
    copy(begin(data), end(data), output_iterator);
    output_file.close();
}

void FileToIntVec(vector<int> &data, const string &fileName)
{
    ifstream input_file;
    input_file.exceptions(ifstream::failbit | ifstream::badbit);
    input_file.open(fileName);

    try
    {
        istream_iterator<int> start(input_file), end;
        data.insert(data.begin(), start, end);
    }
    catch (const ifstream::failure &e)
    {
        if (!input_file.eof())
            throw e;
    }

    input_file.close();
}

void StrVecToFile(const vector<string> &data, const string &fileName)
{
    ofstream output_file;
    output_file.exceptions(ofstream::failbit | ofstream::badbit);
    output_file.open(fileName);

    ostream_iterator<string> output_iterator(output_file, "\n");
    copy(begin(data), end(data), output_iterator);
    output_file.close();
}

void FileToStrVec(vector<string> &data, const string &fileName)
{
    ifstream input_file;
    input_file.exceptions(ifstream::failbit | ifstream::badbit);
    input_file.open(fileName);

    try
    {
        istream_iterator<string> start(input_file), end;
        data.insert(data.begin(), start, end);
    }
    catch (const ifstream::failure &e)
    {
        if (!input_file.eof())
            throw e;
    }
    input_file.close();
}

void NumToFile(const int num, const string &fileName)
{
    ofstream output_file;
    output_file.exceptions(ofstream::failbit | ofstream::badbit);
    output_file.open(fileName);

    output_file << num << '\n';

    output_file.close();
}

void LongLongIntToFile(const long long int num, const string &fileName)
{
    ofstream output_file;
    output_file.exceptions(ofstream::failbit | ofstream::badbit);
    output_file.open(fileName);

    output_file << num << '\n';

    output_file.close();
}

void FileToNum(int &num, const string &fileName)
{
    ifstream input_file;
    input_file.exceptions(ifstream::failbit | ifstream::badbit);
    input_file.open(fileName);

    input_file >> num;

    input_file.close();
}

void FileToLongLongInt(long long int &num, const string &fileName)
{
    ifstream input_file;
    input_file.exceptions(ifstream::failbit | ifstream::badbit);
    input_file.open(fileName);

    input_file >> num;

    input_file.close();
}

// s - string over {0,1,2,3}
void Cont0123(const string &s, vector<char> &cont0123)
{
    cont0123 = vector<char>(4);
    for (char c : s)
    {
        cont0123[c - 48]++;
    }
}

vector<vector<char>> Cont0123(const vector<string> &vec)
{
    vector<vector<char>> result(vec.size());
    for (int i = 0; i < (int)vec.size(); i++)
    {
        Cont0123(vec[i], result[i]);
    }
    return result;
}

string MakeStrand0123(const unsigned length, mt19937 &generator)
{
    string strand;
    uniform_int_distribution<int> distribution(0, 3);
    string letters("0123");
    for (unsigned i = 0; i < length; i++)
    {
        strand += letters[distribution(generator)];
    }
    return strand;
}

// =================================================================================
// SECTION: LINEAR CODE VECTOR MANAGEMENT
// =================================================================================

VectorMode parseVectorMode(const string &mode_str, const string &vector_name)
{
    string lower_mode = mode_str;
    transform(lower_mode.begin(), lower_mode.end(), lower_mode.begin(), ::tolower);

    if (lower_mode == "default" || lower_mode == "identity")
        return VectorMode::DEFAULT;
    if (lower_mode == "random")
        return VectorMode::RANDOM;
    if (lower_mode == "manual")
        return VectorMode::MANUAL;

    throw runtime_error("Invalid mode '" + mode_str + "' for " + vector_name +
                        ". Must be: default/identity, random, or manual");
}

vector<int> parseCSVVector(const string &csv_str, const string &vector_name)
{
    vector<int> result;
    stringstream ss(csv_str);
    string token;

    while (getline(ss, token, ','))
    {
        // Trim whitespace
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);

        if (token.empty())
            continue;

        try
        {
            result.push_back(stoi(token));
        }
        catch (...)
        {
            throw runtime_error("Invalid integer '" + token + "' in " + vector_name);
        }
    }

    return result;
}

vector<int> generateIdentityPerm(int size)
{
    vector<int> perm(size);
    iota(perm.begin(), perm.end(), 0); // [0, 1, 2, ..., size-1]
    return perm;
}

vector<int> generateRandomPerm(int size, mt19937 &rng)
{
    vector<int> perm = generateIdentityPerm(size);
    shuffle(perm.begin(), perm.end(), rng);
    return perm;
}

vector<int> generateRandomBias(int size, mt19937 &rng)
{
    vector<int> bias(size);
    uniform_int_distribution<int> dist(0, 3);
    for (int i = 0; i < size; ++i)
    {
        bias[i] = dist(rng);
    }
    return bias;
}

void validatePermutation(const vector<int> &perm, int expected_size, const string &vector_name)
{
    if ((int)perm.size() != expected_size)
    {
        throw runtime_error(vector_name + " has size " + to_string(perm.size()) + ", expected " +
                            to_string(expected_size));
    }

    vector<bool> seen(expected_size, false);
    for (int val : perm)
    {
        if (val < 0 || val >= expected_size)
        {
            throw runtime_error(vector_name + " contains invalid value " + to_string(val) + ", must be in [0, " +
                                to_string(expected_size - 1) + "]");
        }
        if (seen[val])
        {
            throw runtime_error(vector_name + " contains duplicate value " + to_string(val));
        }
        seen[val] = true;
    }
}

void validateBias(const vector<int> &bias, int expected_size)
{
    if ((int)bias.size() != expected_size)
    {
        throw runtime_error("Bias vector has size " + to_string(bias.size()) + ", expected " +
                            to_string(expected_size));
    }

    for (int val : bias)
    {
        if (val < 0 || val > 3)
        {
            throw runtime_error("Bias contains invalid GF(4) value " + to_string(val) + ", must be in [0, 3]");
        }
    }
}

int calculateRowPermSize(int code_len, int min_hd)
{
    // k = dimension of code
    switch (min_hd)
    {
    case 2:
        return code_len - 1;
    case 3:
        return code_len - 3;
    case 4:
        return code_len - 5;
    case 5:
        return code_len - 7;
    default:
        throw runtime_error("Invalid minHD " + to_string(min_hd) + ", must be 2-5");
    }
}

int calculateBinaryRowPermSize(int code_len, int min_hd)
{
    // k = dimension of binary code
    // For distance 2: k = n - 1
    // For distance 3 (Hamming): k = n - r, where 2^r - 1 >= n
    // For distance 4 (Extended Hamming): k = n - r - 1
    // For distance 5-7 (BCH): code-specific
    switch (min_hd)
    {
    case 2:
        return code_len - 1;
    case 3:
    {
        // Hamming code: find smallest r such that 2^r - 1 >= code_len
        int r = 1;
        while ((1 << r) - 1 < code_len)
            r++;
        return code_len - r;
    }
    case 4:
    {
        // Extended Hamming: find smallest r such that 2^r >= code_len
        int r = 1;
        while ((1 << r) < code_len)
            r++;
        return code_len - r - 1;
    }
    case 5:
    {
        // BCH d=5: g(x) = lcm(M1, M3), degree = 2*r for GF(2^r)
        int r = 1;
        while ((1 << r) - 1 < code_len)
            r++;
        return code_len - 2 * r;
    }
    case 6:
    {
        // Extended BCH d=5 + parity -> d=6
        int r = 1;
        while ((1 << r) - 1 < code_len - 1)
            r++;
        return (code_len - 1) - 2 * r;
    }
    case 7:
    {
        // BCH d=7: g(x) = lcm(M1, M3, M5), degree = 3*r for GF(2^r)
        int r = 1;
        while ((1 << r) - 1 < code_len)
            r++;
        return code_len - 3 * r;
    }
    default:
        throw runtime_error("Invalid binary minHD " + to_string(min_hd) + ", must be 2-7");
    }
}

void validateBinaryBias(const vector<int> &bias, int expected_size)
{
    if ((int)bias.size() != expected_size)
    {
        throw runtime_error("Binary bias vector has size " + to_string(bias.size()) + ", expected " +
                            to_string(expected_size));
    }

    for (int val : bias)
    {
        if (val < 0 || val > 1)
        {
            throw runtime_error("Binary bias contains invalid value " + to_string(val) + ", must be in {0, 1}");
        }
    }
}

vector<int> generateRandomBinaryBias(int size, mt19937 &rng)
{
    vector<int> bias(size);
    uniform_int_distribution<int> dist(0, 1);
    for (int i = 0; i < size; ++i)
    {
        bias[i] = dist(rng);
    }
    return bias;
}
#include "json.hpp"
using json = nlohmann::json;

void LoadParamsFromJson(Params &params, const std::string &filename)
{
    std::ifstream f(filename);
    if (!f.is_open())
        throw std::runtime_error("Could not open config file: " + filename);

    json j;
    f >> j;

    // Core
    if (j.contains("core")) {
        auto& c = j["core"];
        if (c.contains("lenStart")) params.codeLen = c["lenStart"]; // Note: logic in main sets loops, here we just set base struct
        if (c.contains("editDist")) params.codeMinED = c["editDist"];
    }

    // Constraints
    if (j.contains("constraints")) {
        auto& c = j["constraints"];
        if (c.contains("maxRun")) params.maxRun = c["maxRun"];
        if (c.contains("minGC")) params.minGCCont = c["minGC"];
        if (c.contains("maxGC")) params.maxGCCont = c["maxGC"];
    }

    // Performance
    if (j.contains("performance")) {
        auto& p = j["performance"];
        if (p.contains("threads")) params.threadNum = p["threads"];
        if (p.contains("saveInterval")) params.saveInterval = p["saveInterval"];
    if (p.contains("use_gpu")) params.useGPU = p["use_gpu"];
    if (p.contains("max_gpu_memory_gb")) params.maxGPUMemoryGB = p["max_gpu_memory_gb"];
    if (p.contains("do_clustering")) { // Backwards compatibility or manual override in performance
         params.clustering.enabled = p["do_clustering"];
    }
    }
    
    // Clustering
    if (j.contains("clustering")) {
        auto& c = j["clustering"];
        if (c.contains("enabled")) params.clustering.enabled = c["enabled"];
        if (c.contains("k")) params.clustering.k = c["k"];
        if (c.contains("verbose")) params.clustering.verbose = c["verbose"];
        if (c.contains("convergenceIterations")) params.clustering.convergenceIterations = c["convergenceIterations"];
        if (c.contains("method")) params.clustering.method = c["method"];
    }
    
    // Verify
    if (j.contains("verify")) params.verify = j["verify"];

    // Method
    if (j.contains("method")) {
        auto& m = j["method"];
        std::string name = m["name"];
        
        if (name == "LinearCode") {
            params.method = GenerationMethod::LINEAR_CODE;
            auto& lc = m["linearCode"];
            int minHD = lc.contains("minHD") ? (int)lc["minHD"] : 3;
            
            VectorMode biasMode = VectorMode::DEFAULT;
            VectorMode rowMode = VectorMode::DEFAULT;
            VectorMode colMode = VectorMode::DEFAULT;
            
            if (lc.contains("biasMode")) biasMode = parseVectorMode(lc["biasMode"], "bias");
            if (lc.contains("rowPermMode")) rowMode = parseVectorMode(lc["rowPermMode"], "row_perm");
            if (lc.contains("colPermMode")) colMode = parseVectorMode(lc["colPermMode"], "col_perm");
            
            std::vector<int> bias, row, col;
            if (lc.contains("bias")) bias = lc["bias"].get<std::vector<int>>();
            if (lc.contains("rowPerm")) row = lc["rowPerm"].get<std::vector<int>>();
            if (lc.contains("colPerm")) col = lc["colPerm"].get<std::vector<int>>();
            
            unsigned int seed = lc.contains("randomSeed") ? (unsigned int)lc["randomSeed"] : 0;
            
            params.constraints = std::make_unique<LinearCodeConstraints>(minHD, biasMode, rowMode, colMode, bias, row, col, seed);
        }
        else if (name == "VTCode") {
            params.method = GenerationMethod::VT_CODE;
            int a = m["vtCode"].contains("a") ? (int)m["vtCode"]["a"] : 0;
            int b = m["vtCode"].contains("b") ? (int)m["vtCode"]["b"] : 0;
            params.constraints = std::make_unique<VTCodeConstraints>(a, b);
        }
        else if (name == "Random") {
            params.method = GenerationMethod::RANDOM;
            int c = m["random"].contains("candidates") ? (int)m["random"]["candidates"] : 10000;
            params.constraints = std::make_unique<RandomConstraints>(c);
        }
        else if (name == "Diff_VTCode") {
            params.method = GenerationMethod::DIFFERENTIAL_VT_CODE;
            int s = m["diffVtCode"].contains("syndrome") ? (int)m["diffVtCode"]["syndrome"] : 0;
            params.constraints = std::make_unique<DifferentialVTCodeConstraints>(s);
        }
        else if (name == "AllStrings") {
            params.method = GenerationMethod::ALL_STRINGS;
            params.constraints = std::make_unique<AllStringsConstraints>();
        }
        else if (name == "FileRead") {
            params.method = GenerationMethod::FILE_READ;
            std::string inputFile = m["fileRead"].contains("input_file") ? (std::string)m["fileRead"]["input_file"] : "";
            if (inputFile.empty()) {
                 throw std::runtime_error("FileRead method requires 'input_file' parameter in config.");
            }
            params.constraints = std::make_unique<FileReadConstraints>(inputFile);
        }
        else if (name == "LinearBinaryCode") {
            params.method = GenerationMethod::LINEAR_BINARY_CODE;
            auto& lbc = m["linearBinaryCode"];
            int minHD = lbc.contains("minHD") ? (int)lbc["minHD"] : 3;
            
            VectorMode biasMode = VectorMode::DEFAULT;
            VectorMode rowMode = VectorMode::DEFAULT;
            VectorMode colMode = VectorMode::DEFAULT;
            
            if (lbc.contains("biasMode")) biasMode = parseVectorMode(lbc["biasMode"], "bias");
            if (lbc.contains("rowPermMode")) rowMode = parseVectorMode(lbc["rowPermMode"], "row_perm");
            if (lbc.contains("colPermMode")) colMode = parseVectorMode(lbc["colPermMode"], "col_perm");
            
            std::vector<int> bias, row, col;
            if (lbc.contains("bias")) bias = lbc["bias"].get<std::vector<int>>();
            if (lbc.contains("rowPerm")) row = lbc["rowPerm"].get<std::vector<int>>();
            if (lbc.contains("colPerm")) col = lbc["colPerm"].get<std::vector<int>>();
            
            unsigned int seed = lbc.contains("randomSeed") ? (unsigned int)lbc["randomSeed"] : 0;
            
            params.constraints = std::make_unique<LinearBinaryCodeConstraints>(minHD, biasMode, rowMode, colMode, bias, row, col, seed);
        }
    }
}

template<typename T>
string NumberWithCommas(T value)
{
    std::stringstream ss;
    ss << value;
    string s = ss.str();
    if (s.length() <= 3) return s;
    
    // Insert comma every 3 digits from right
    string res = "";
    int count = 0;
    for (int i = s.length() - 1; i >= 0; i--) {
        res = s[i] + res;
        count++;
        if (count == 3 && i > 0) {
            res = "," + res;
            count = 0;
        }
    }
    return res;
}

// Explicit instantiation for likely types
template string NumberWithCommas<int>(int);
template string NumberWithCommas<long>(long);
template string NumberWithCommas<long long>(long long);
template string NumberWithCommas<unsigned long>(unsigned long);
