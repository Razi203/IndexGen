//void FilterByDistONMEM(vector<string>& vecs, const int minED, DistFunc distFunc) {
//	int n = vecs.size();
//	vector<int> idxs;
//	for (int i = 0; i < n; i++) {
//		idxs.push_back(i);
//	}
//
//	vector<int> sm = SumRowsONMEM(vecs, minED, distFunc);
//	while (1) {
//		int maxIdx = max_element(sm.begin(), sm.end()) - sm.begin();
//		if (sm[maxIdx] == 0)
//			break;
//		SubColONMEM(sm, vecs, minED, idxs, maxIdx, distFunc);
//
//		sm.erase(sm.begin() + maxIdx);
//		vecs.erase(vecs.begin() + maxIdx);
//		idxs.erase(idxs.begin() + maxIdx);
//	}
//}

// Create a set of codeLen length strings over {0,1,2,3} with pairwise minimum edit distance of minED
// Old Algorithm - Max:
// C - a set of candidate strings of length codeLen.
// create matrix M: M(i,j) = 1 if ED(C(i), C(j)) < minED. Otherwise, 0.
// while M is not the zero matrix: let k be the index of the row of M with maximal sum, delete column and row k of M.
// return the candidate strings corresponding to the rows left in M.

// candidate strings: generate strings with min Hamming distance of 3 by Hamming codes. Filter by GCR.

//void GenerateCodebookMax(const int codeLen, const int maxRun, const double minGCCont, const double maxGCCont,
//		const int minED) {
//
//	clock_t begin = clock();
//	string testName = "Hamming Max";
//	PrintTestParams(codeLen, maxRun, minGCCont, maxGCCont, minED, testName);
//
//// generate candidate strings
//	vector<string> filteredHamming = GenAllGCRFilteredCodeStrings(codeLen, 3, maxRun, minGCCont, maxGCCont);
//	cout << "number of filtered vectors:\t" << filteredHamming.size() << endl;
//
//// filter by edit distance
//	FilterByDistONMEM(filteredHamming, minED, EditDistanceT);
//	cout << "number of code vecs:\t\t" << filteredHamming.size() << endl;
//
//	clock_t end = clock();
//	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
//	cout << "Index Generation Time:\t" << fixed << setprecision(2) << elapsed_secs << "\tseconds" << endl;
//
//	ToFile(filteredHamming, testName, codeLen, maxRun, minGCCont, maxGCCont, minED, GEN_WO_I, MAX_DATA_LEN, REDUNDANCY);
//
////	VerifyDist(filteredHamming, minED);
//	cout << "=====================================================" << endl;
//}

// Max algorithm
// candidate strings: all possible GCR filtered strings of len codeLen over {0,1,2,3}

//void GenerateCodebookMaxNoHamm(const int codeLen, const int maxRun, const double minGCCont, const double maxGCCont,
//		const int minED) {
//
//	clock_t begin = clock();
//	string testName = "No Hamming Max";
//	PrintTestParams(codeLen, maxRun, minGCCont, maxGCCont, minED, testName);
//
//// generate candidate strings
//	vector<string> filteredHamming = GenAllGCRFilteredStrings(codeLen, maxRun, minGCCont, maxGCCont);
//	cout << "number of filtered vectors:\t" << filteredHamming.size() << endl;
//
//// filter by edit distance
//	FilterByDistONMEM(filteredHamming, minED, EditDistanceT);
//	cout << "number of code vecs:\t\t" << filteredHamming.size() << endl;
//
//	clock_t end = clock();
//	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
//	cout << "Index Generation Time:\t" << fixed << setprecision(2) << elapsed_secs << "\tseconds" << endl;
//
//	ToFileNoHamm(filteredHamming, testName, codeLen, maxRun, minGCCont, maxGCCont, minED);
//
////	VerifyDist(filteredHamming, minED);
//	cout << "=====================================================" << endl;
//}
//void SubColT(vector<int>& sumRows, const vector<string>& vecs, const vector<int>& idxs, const int colIdx,
//		const int minED, const int threadIdx, const int threadNum, DistFunc distFunc) {
//	int n = vecs.size();
//	for (int i = threadIdx; i < n; i += threadNum) {
//		if (i == colIdx)
//			continue;
//		bool distIsAtLeastMin = distFunc(vecs[i], vecs[colIdx], minED, threadIdx, threadNum);
//		if (!distIsAtLeastMin) {
//			sumRows[i]--;
//		}
//	}
//	return;
//}
//
//void SubColONMEM(vector<int>& sumRows, const vector<string>& vecs, const int minED, const vector<int>& idxs,
//		const int colIdx, DistFunc distFunc) {
//	vector<thread> threads;
//	for (int i = 0; i < THREAD_NUM; i++) {
//		threads.push_back(thread(SubColT, ref(sumRows), ref(vecs), idxs, colIdx, minED, i, THREAD_NUM, distFunc));
//	}
//	for (thread& th : threads)
//		th.join();
//}
//
//void SumRowT(const vector<string>& vecs, vector<int>& threadSumRows, const int minED, const int threadIdx,
//		const int threadNum, DistFunc distFunc) {
//	for (int i = threadIdx; i < (int) vecs.size(); i += threadNum) {
//		for (int j = i + 1; j < (int) vecs.size(); j++) {
//			bool distIsAtLeastMin = distFunc(vecs[i], vecs[j], minED, threadIdx, threadNum);
//			if (!distIsAtLeastMin) {
//				threadSumRows[i]++;
//				threadSumRows[j]++;
//			}
//		}
//	}
//}
//
//vector<int> SumRowsONMEM(const vector<string>& vecs, const int minED, DistFunc distFunc) {
//	vector<vector<int>> threadSumRows(THREAD_NUM, vector<int>(vecs.size()));
//	vector<thread> threads;
//	for (int i = 0; i < THREAD_NUM; i++) {
//		threads.push_back(thread(SumRowT, ref(vecs), ref(threadSumRows[i]), minED, i, THREAD_NUM, distFunc));
//	}
//	for (thread& th : threads)
//		th.join();
//
//	vector<int> sumRows(vecs.size());
//	for (int i = 0; i < THREAD_NUM; i++) {
//		for (int j = 0; j < (int) vecs.size(); j++) {
//			sumRows[j] += threadSumRows[i][j];
//		}
//	}
//	return sumRows;
//}
// Min algorithm for minED > 3
// C1 - candidate strings with min Hamming distance of 3
// 2 passes:
// pass 1: use Min algorithm to get C2, candidates with min *Hamming distance* minED, from C1
// pass 2: use Min algorithm to get G, strings with min edit distance minED, from C2

//void GenerateCodebookMin2Pass(const int codeLen, const int maxRun, const double minGCCont, const double maxGCCont,
//		const int minED, const int saveInterval) {
//	assert(minED > 3);
//	try {
//		clock_t begin = clock();
//		string testName = "HammingMin2Pass";
//		ParamsToFile(testName, codeLen, maxRun, minGCCont, maxGCCont, minED, saveInterval, "progress_params.txt");
//		PrintTestParams(codeLen, maxRun, minGCCont, maxGCCont, minED, testName);
//
//		// generate candidate strings with min hamming dist 3
//		vector<string> candidates = GenAllGCRFilteredCodeStrings(codeLen, 3, maxRun, minGCCont, maxGCCont);
//		StrVecToFile(candidates, "progress_cand.txt");
//		cout << "number of candidate strings:\t" << candidates.size() << endl;
//
//		// Pass 1
//		// filter candidate strings to get candidates with min Hamming dist minED
//		vector<string> candidates2;
//		NumToFile(1, "progress_pass.txt");
//		FilterByDistMinONMEM(candidates, minED, candidates2, saveInterval, HammingDist);
//		StrVecToFile(candidates2, "progress_cand2.txt");
//		cout << "number of min hamm " << minED << " strings:\t" << candidates2.size() << endl;
//
//		// Pass 2
//		// filter minHammCandidates to get codebook with min Edit dist minED
//		vector<string> codebook;
//		NumToFile(2, "progress_pass.txt");
//		FilterByDistMinONMEM(candidates2, minED, codebook, saveInterval, EditDistanceT);
//		cout << "number of code strings:\t\t" << codebook.size() << endl;
//
//		clock_t end = clock();
//		double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
//		cout << "Index Generation Time:\t" << fixed << setprecision(2) << elapsed_secs << "\tseconds" << endl;
//
//		ToFile(codebook, testName, codeLen, maxRun, minGCCont, maxGCCont, minED, GEN_WO_I, MAX_DATA_LEN, REDUNDANCY);
//		VerifyDist(codebook, minED);
//
//		cout << "=====================================================" << endl;
//		remove("progress_params.txt");
//		remove("progress_cand.txt");
//		remove("progress_cand2.txt");
//		remove("progress_pass.txt");
//	} catch (const ofstream::failure& e) {
//		cout << "Can't write one or more progress files or a writing error occurred! Aborted." << endl;
//	}
//}

//void GenerateCodebookMin2PassResumeFromFile() {
//	try {
//		string testName;
//		int codeLen, maxRun, minED, saveInterval;
//		double minGCCont, maxGCCont;
//		FileToParams(testName, codeLen, maxRun, minGCCont, maxGCCont, minED, saveInterval, "progress_params.txt");
//		cout << "Resuming from file" << endl;
//		PrintTestParams(codeLen, maxRun, minGCCont, maxGCCont, minED, testName);
//
//		int pass;
//		FileToNum(pass, "progress_pass.txt");
//
//		vector<string> candidates, candidates2, codebook;
//		if (pass == 1) {
//			cout << "Resuming pass 1" << endl;
//			FileToStrVec(candidates, "progress_cand.txt");
//			cout << "number of candidate strings:\t" << candidates.size() << endl;
//			FilterByDistMinONMEMResumeFromFile(candidates, minED, candidates2, saveInterval, HammingDist);
//			cout << "number of min hamm " << minED << " strings:\t" << candidates2.size() << endl;
//			StrVecToFile(candidates2, "progress_cand2.txt");
//			NumToFile(2, "progress_pass.txt");
//			FilterByDistMinONMEM(candidates2, minED, codebook, saveInterval, EditDistanceT);
//
//		}
//		else {
//			assert(pass == 2);
//			cout << "Resuming pass 2" << endl;
//			FileToStrVec(candidates2, "progress_cand2.txt");
//			cout << "number of min hamm " << minED << " strings:\t" << candidates2.size() << endl;
//			FilterByDistMinONMEMResumeFromFile(candidates2, minED, codebook, saveInterval, EditDistanceT);
//		}
//
//		cout << "number of code strings:\t\t" << codebook.size() << endl;
//		ToFile(codebook, testName, codeLen, maxRun, minGCCont, maxGCCont, minED, GEN_WO_I, MAX_DATA_LEN, REDUNDANCY);
//		cout << "=====================================================" << endl;
//		remove("progress_params.txt");
//		remove("progress_cand.txt");
//		remove("progress_cand2.txt");
//		remove("progress_pass.txt");
//	} catch (const ifstream::failure& e) {
//		cout << "Read/Write progress files error! Aborted." << endl;
//	}
//}

//void SumRowsTime(const int codeLen, const int maxRun, const double minGCCont, const double maxGCCont, const int minED,
//		DistFunc distFunc) {
//	vector<string> candidates = GenAllGCRFilteredCodeStrings(codeLen, 3, maxRun, minGCCont, maxGCCont);
//	clock_t start = clock();
//	vector<int> sumRows = SumRowsONMEM(candidates, minED, distFunc);
//	clock_t end = clock();
//	double elapsed_secs = double(end - start) / CLOCKS_PER_SEC;
//	cout << "Sum Rows Time: " << fixed << setprecision(2) << elapsed_secs << "\tseconds" << endl;
//}

// candidate strings: all possible GCR filtered strings of len codeLen over {0,1,2,3}

//void GenerateCodebookMinNoHamm(const Params& params) {
//
//	try {
//		clock_t begin = clock();
//		string testName = "NoHammingMin";
//		ParamsToFile(testName, params, "progress_params.txt");
//		PrintTestParams(params, testName);
//
//// generate candidate strings
//		vector<string> candidates = GenAllGCRFilteredStrings(params);
//		StrVecToFile(candidates, "progress_cand.txt");
//
//		cout << "number of candidate strings:\t" << candidates.size() << endl;
//
//// filter by edit distance
//		vector<string> codebook;
//		int candidateNum = candidates.size();
//		long long int matrixOnesNum;
//		FilterByDistMinONMEM(candidates, codebook, params, EditDistanceT, matrixOnesNum);
//		cout << "Number of Ones in Matrix:\t" << matrixOnesNum << endl;
//		long long int candSqr = (long long int) candidateNum * (long long int) candidateNum;
//		long double matrixOnesFrac = (long double) matrixOnesNum / (long double) candSqr;
//		cout << "Ones Fraction in Matrix:\t" << matrixOnesFrac << endl;
//		cout << "Number of code strings:\t\t" << codebook.size() << endl;
//
//		clock_t end = clock();
//		double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
//		cout << "Index Generation Time:\t" << fixed << setprecision(2) << elapsed_secs << "\tseconds" << endl;
//
//		ToFile(codebook, testName, params, candidateNum, matrixOnesNum, matrixOnesFrac);
//
//		VerifyDist(codebook, params.minED, params.maxCodeLen, params.threadNum);
//		cout << "=====================================================" << endl;
//		remove("progress_params.txt");
//		remove("progress_cand.txt");
//	} catch (const ofstream::failure& e) {
//		cout << "Can't write one or more progress files or a writing error occurred! Aborted." << endl;
//	}
//}

// If execution of GenerateCodebookMinNoHamm was not completed resume from file.

//void GenerateCodebookMinNoHammResumeFromFile() {
//	try {
//		string testName;
//		Params params;
//		FileToParams(testName, params, "progress_params.txt");
//		cout << "Resuming from file" << endl;
//		PrintTestParams(params, testName);
//		vector<string> candidates;
//		FileToStrVec(candidates, "progress_cand.txt");
//		cout << "number of candidate strings:\t" << candidates.size() << endl;
//		vector<string> codebook;
//		int candidateNum = candidates.size();
//		long long int matrixOnesNum;
//		FilterByDistMinONMEMResumeFromFile(candidates, codebook, params, EditDistanceT, matrixOnesNum);
//		cout << "Number of Ones in Matrix:\t" << matrixOnesNum << endl;
//		long long int candSqr = (long long int) candidateNum * (long long int) candidateNum;
//		long double matrixOnesFrac = (long double) matrixOnesNum / (long double) candSqr;
//		cout << "Ones Fraction in Matrix:\t" << matrixOnesFrac << endl;
//		cout << "number of code strings:\t\t" << codebook.size() << endl;
//		ToFile(codebook, testName, params, candidateNum, matrixOnesNum, matrixOnesFrac);
//		VerifyDist(codebook, params.minED, params.maxCodeLen, params.threadNum);
//		cout << "=====================================================" << endl;
//		remove("progress_params.txt");
//		remove("progress_cand.txt");
//	} catch (const ifstream::failure& e) {
//		cout << "Read/Write progress files error! Aborted." << endl;
//	}
//}
//// All strings over {A,C,G,T} of length n filtered by maxRun and GC content (NO MINIMUM HAMMING DISTANCE)
//vector<string> GenAllGCRFilteredStrings(const Params& params) {
//	vector<string> result;
//	vector<int> start(params.codeLen, 0);
//	for (vector<int> vec = start; not vec.empty(); vec = NextBase4(vec)) {
//		string curr = VecToStr(vec);
//		if ((MaxRun(curr) <= params.maxRun) && TestGCCont(curr, params.minGCCont, params.maxGCCont) && TestAllLettersOccurence(curr)) {
//			result.push_back(curr);
//		}
//	}
//	return result;
//}

// All strings over {A,C,G,T} of length n with pairwise minimum Hamming distance minHammDist {3,4,5} filtered by maxRun and GC content
//n: length of codedVecs:	minHammDist 3: 4<=n<=21
//							minHammDist 4: 6<=n<=41
//							minHammDist 5: 8<=n<=43
//vector<string> GenAllGCRFilteredCodeStrings(const Params& params) {
//	vector<vector<int> > codeVecs = CodedVecs(params.codeLen, params.minHD);
//	vector<string> codeWords;
//	for (vector<int>& vec : codeVecs) {
//		codeWords.push_back(VecToStr(vec));
//	}
//	vector<string> filteredCodeWords;
//	FilterByGCR(codeWords, filteredCodeWords, params.maxRun, params.minGCCont, params.maxGCCont);
//	return filteredCodeWords;
//}

//struct EDParams {
//	int delNum;
//	int insNum;
//	int subNum;
//	int xPos;
//	int yPos;
//	EDParams(const int delNum, const int insNum, const int subNum, const int xPos, const int yPos) :
//			delNum(delNum), insNum(insNum), subNum(subNum), xPos(xPos), yPos(yPos) {
//	}
//};
//
//bool EditByOps(const string& x, const string& y, const int delNum, const int insNum, const int subNum) {
//	EDParams p(delNum, insNum, subNum, 0, 0);
//	int xLen = x.length(), yLen = y.length();
//	stack<EDParams> stack;
//	stack.push(p);
//	while (not stack.empty()) {
//		p = stack.top();
//		stack.pop();
//		if (p.xPos == xLen and p.yPos == yLen) {
//			return true;
//		}
//		else if (p.xPos == xLen) {
//			if (p.insNum >= yLen - p.yPos)
//				return true;
//			else
//				continue;
//		}
//		else if (p.yPos == yLen) {
//			if (p.delNum >= xLen - p.xPos)
//				return true;
//			else
//				continue;
//		}
//
//		if (x[p.xPos] == y[p.yPos]) {
//			p.xPos++;
//			p.yPos++;
//			stack.push(p);
//		}
//		else {
//
//			// del
//			if (p.delNum > 0) {
//				stack.push(EDParams(p.delNum - 1, p.insNum, p.subNum, p.xPos + 1, p.yPos));
//			}
//
//			// ins
//			if (p.insNum > 0) {
//				stack.push(EDParams(p.delNum, p.insNum - 1, p.subNum, p.xPos, p.yPos + 1));
//			}
//
//			// sub
//			if (p.subNum > 0) {
//				stack.push(EDParams(p.delNum, p.insNum, p.subNum - 1, p.xPos + 1, p.yPos + 1));
//			}
//		}
//	}
//	return false;
//}

//void PrintRes(const bool cond, const string& testName) {
//	if (cond) {
//		cout << testName << " Success" << endl;
//	}
//	else {
//		cout << testName << " Failure" << endl;
//	}
//}
//
//void Test1Del() {
//	int delNum = 1;
//	int insNum = 0;
//	int subNum = 0;
//	string x = "AB";
//	string y = "A";
//	bool result1 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result1, "1 Del 1");
//
//	x = "AB";
//	y = "B";
//	bool result2 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result2, "1 Del 2");
//
//	x = "ABC";
//	y = "AC";
//	bool result3 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result3, "1 Del 3");
//
//	x = "A";
//	y = "";
//
//	bool result4 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result4, "1 Del 4");
//
//	x = "AB";
//	y = "A";
//	delNum = 0;
//	bool result5 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(!result5, "1 Del 5");
//
//	x = "ABC";
//	y = "A";
//	delNum = 1;
//	bool result6 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(!result6, "1 Del 6");
//
//	x = "A";
//	y = "AB";
//	delNum = 1;
//	bool result7 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(!result7, "1 Del 7");
//
//	x = "A";
//	y = "B";
//	delNum = 1;
//	bool result8 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(!result8, "1 Del 8");
//
//	x = "AB";
//	y = "AB";
//	delNum = 1;
//	bool result9 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result9, "1 Del 9");
//}
//
//void Test1Ins() {
//	int delNum = 0;
//	int insNum = 1;
//	int subNum = 0;
//	string x = "A";
//	string y = "AB";
//	bool result1 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result1, "1 Ins 1");
//
//	x = "B";
//	y = "AB";
//	bool result2 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result2, "1 Ins 2");
//
//	x = "AC";
//	y = "ABC";
//	bool result3 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result3, "1 Ins 3");
//
//	x = "";
//	y = "A";
//
//	bool result4 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result4, "1 Ins 4");
//
//	x = "A";
//	y = "AB";
//	insNum = 0;
//	bool result5 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(!result5, "1 Ins 5");
//
//	x = "A";
//	y = "ABC";
//	insNum = 1;
//	bool result6 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(!result6, "1 Ins 6");
//
//	x = "AB";
//	y = "A";
//	insNum = 1;
//	bool result7 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(!result7, "1 Ins 7");
//
//	x = "A";
//	y = "B";
//	insNum = 1;
//	bool result8 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(!result8, "1 Ins 8");
//
//	x = "AB";
//	y = "AB";
//	insNum = 1;
//	bool result9 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result9, "1 Ins 9");
//}
//
//void Test1Sub() {
//	int delNum = 0;
//	int insNum = 0;
//	int subNum = 1;
//	string x = "A";
//	string y = "B";
//	bool result1 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result1, "1 Sub 1");
//
//	x = "ABC";
//	y = "BBC";
//
//	bool result2 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result2, "1 Sub 2");
//
//	x = "ABC";
//	y = "ADC";
//
//	bool result3 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result3, "1 Sub 3");
//
//	x = "ABC";
//	y = "ABD";
//
//	bool result4 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result4, "1 Sub 4");
//
//	x = "AB";
//	y = "BA";
//
//	bool result5 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(!result5, "1 Sub 5");
//
//	x = "A";
//	y = "B";
//	subNum = 0;
//	bool result6 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(!result6, "1 Sub 6");
//
//	x = "ABC";
//	y = "ABC";
//	subNum = 0;
//	bool result7 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result7, "1 Sub 7");
//
//	x = "AB";
//	y = "A";
//	subNum = 1;
//	bool result8 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(!result8, "1 Sub 8");
//
//	x = "BA";
//	y = "A";
//	subNum = 1;
//	bool result9 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(!result9, "1 Sub 9");
//}
//
//void Test1Ins1Del() {
//	int delNum = 1;
//	int insNum = 1;
//	int subNum = 0;
//	string x = "A";
//	string y = "B";
//	bool result1 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result1, "1 Ins 1 Del 1");
//
//	x = "AB";
//	y = "BA";
//	bool result2 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result2, "1 Ins 1 Del 2");
//
//	x = "AB";
//	y = "BC";
//	bool result3 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result3, "1 Ins 1 Del 3");
//
//	x = "AB";
//	y = "CA";
//	bool result4 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result4, "1 Ins 1 Del 4");
//
//	x = "A";
//	y = "";
//	bool result5 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result5, "1 Ins 1 Del 5");
//
//	x = "";
//	y = "A";
//	bool result6 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result6, "1 Ins 1 Del 6");
//
//	x = "AAACA";
//	y = "ABAAA";
//	bool result7 = EditByOps(x, y, delNum, insNum, subNum);
//	PrintRes(result7, "1 Ins 1 Del 7");
//}

//void TestHammCont(const Params& params) {
//	vector<string> candidates = Candidates(params);
//	vector<vector<char> > cont0123 = Cont0123z(candidates);
//	int codeLen = params.codeLen;
//	int candidateNum = candidates.size();
//	cout << "Candidate Num:\t" << candidateNum << endl;
//	vector<vector<int> > occurences(codeLen + 1, vector<int>(codeLen * 2 + 1));
//	vector<vector<int> > ones(codeLen + 1, vector<int>(codeLen * 2 + 1));
//	for (int i = 0; i < candidateNum; i++) {
//		for (int j = i + 1; j < candidateNum; j++) {
//			int hammDist = HammingDist(candidates[i], candidates[j]);
//			assert(hammDist <= codeLen);
//			int sumAbsDiff = SumAbs0123Diffz(cont0123[i], cont0123[j]);
//			assert(sumAbsDiff <= (2 * codeLen));
//			bool atLeastMin = EditDistance(candidates[i], candidates[j], params.minED, params.maxCodeLen);
//			occurences[hammDist][sumAbsDiff]++;
//			if (!atLeastMin)
//				ones[hammDist][sumAbsDiff]++;
//		}
//	}
//	int w = 10;
//	cout << "    ";
//	for (int i = 0; i < codeLen * 2 + 1; i += 2) {
//		cout << setw(21) << i << " | ";
//	}
//	cout << endl;
//	long long countOnes = 0;
//	for (int i = 0; i < codeLen + 1; i++) {
//		cout << setw(2) << i << ": ";
//		for (int j = 0; j < codeLen * 2 + 1; j += 2) {
//			cout << setw(w) << occurences[i][j] << " " << setw(w) << ones[i][j] << " | ";
//			countOnes += ones[i][j];
//		}
//		cout << endl;
//	}
//	countOnes *= 2;
//	cout << endl;
//	cout << "Ones num: " << countOnes << endl;
//	long long candSqr = candidateNum * candidateNum;
//	long double onesFrac = (long double) countOnes / candSqr;
//	cout << "Ones frac: " << onesFrac << endl;
//}

//void SaveProgressSumRows(const int i, const vector<int>& threadSumRows, const int threadIdx) {
//	string smFilename = "progress_sum_rows_" + to_string(threadIdx) + ".txt";
//	IntVecToFile(threadSumRows, smFilename);
//	string iFilename = "progress_sum_rows_i_" + to_string(threadIdx) + ".txt";
//	NumToFile(i, iFilename);
//}
//
//void DelProgressSumRows(const int threadIdx) {
//	string smFilename = "progress_sum_rows_" + to_string(threadIdx) + ".txt";
//	remove(smFilename.c_str());
//	string iFilename = "progress_sum_rows_i_" + to_string(threadIdx) + ".txt";
//	remove(iFilename.c_str());
//}
//
//void SumRowSave(const vector<string>& candidates, const vector<vector<char> >& cand0123Cont, vector<int>& threadSumRows,
//		const int minED, const int saveInterval, clock_t& threadLastSaveTime, const int threadStart,
//		const int threadIdx, const int threadNum, const int maxCodeLen) {
//	threadLastSaveTime = clock();
//	for (int i = threadStart; i < (int) candidates.size(); i += threadNum) {
//		for (int j = i + 1; j < (int) candidates.size(); j++) {
//			bool distIsAtLeastMin = FastEditDistance0123(candidates[i], candidates[j], minED, cand0123Cont[i],
//					cand0123Cont[j]);
//			if (!distIsAtLeastMin) {
//				threadSumRows[i]++;
//				threadSumRows[j]++;
//			}
//		}
//		clock_t currentTime = clock();
//		int secs_since_last_save = double(currentTime - threadLastSaveTime) / CLOCKS_PER_SEC;
//		if (secs_since_last_save > saveInterval) {
//			SaveProgressSumRows(i, threadSumRows, threadIdx);
//			threadLastSaveTime = currentTime;
//			cout << "Sum Rows PROGRESS: i=" << i << " of " << candidates.size() << "\tthreadId\t" << threadIdx << endl;
//		}
//	}
//}
//
//vector<int> SumRowsONMEM(const vector<string>& candidates, const vector<vector<char> >& cand0123Cont, const int minED,
//		const int saveInterval, const int maxCodeLen, const int threadNum) {
//	vector<vector<int>> threadSumRows(threadNum, vector<int>(candidates.size()));
//	vector<clock_t> threadLastSave(threadNum);
//	vector<thread> threads;
//	for (int i = 0; i < threadNum; i++) {
//		threads.push_back(
//				thread(SumRowSave, ref(candidates), ref(cand0123Cont), ref(threadSumRows[i]), minED, saveInterval,
//						ref(threadLastSave[i]), i, i, threadNum, maxCodeLen));
//	}
//	for (thread& th : threads)
//		th.join();
//	vector<int> sumRows(candidates.size());
//	for (int i = 0; i < threadNum; i++) {
//		for (int j = 0; j < (int) candidates.size(); j++) {
//			sumRows[j] += threadSumRows[i][j];
//		}
//	}
//	for (int i = 0; i < threadNum; i++) {
//		DelProgressSumRows(i);
//	}
//	return sumRows;
//}
//
//void MinSumRowONMEM(const vector<int>& sumRows, const int threadIdx, const int threadNum, pair<int, int>& minPtrVal) {
//	int minVal = INT_MAX;
//	int minPtr = -1;
//	for (int i = threadIdx; i < (int) sumRows.size(); i += threadNum) {
//		if (sumRows[i] < minVal) {
//			minVal = sumRows[i];
//			minPtr = i;
//		}
//		if (minVal == 0) {
//			minPtrVal = make_pair(minPtr, minVal);
//			return;
//		}
//	}
//	assert(minPtr >= 0);
//	minPtrVal = make_pair(minPtr, minVal);
//}
//
//bool PairComp(const pair<int, int>& a, const pair<int, int>& b) {
//	return a.second < b.second;
//}
//
//void MinSumRowONMEMTHW(const vector<int>& sumRows, int& minSumPtr, int& minSumVal, const int threadNum) {
//	if ((int) sumRows.size() < 4 * threadNum) {
//		int minIdx = min_element(sumRows.begin(), sumRows.end()) - sumRows.begin();
//		minSumPtr = minIdx;
//		minSumVal = sumRows[minIdx];
//		return;
//	}
//	vector<pair<int, int> > threadMinPtrVal(threadNum);
//	vector<thread> threads;
//	for (int i = 0; i < threadNum; i++) {
//		threads.push_back(thread(MinSumRowONMEM, ref(sumRows), i, threadNum, ref(threadMinPtrVal[i])));
//	}
//	for (thread& th : threads)
//		th.join();
//	vector<pair<int, int>>::iterator minElemIt = min_element(threadMinPtrVal.begin(), threadMinPtrVal.end(), PairComp);
//	minSumPtr = minElemIt->first;
//	minSumVal = minElemIt->second;
//}
//
//void InBallOfMin(const vector<string>& candidates, const vector<vector<char> >& cand0123Cont, const int minED,
//		const int minSumPtr, const vector<int>& remainingStrsPtrs, vector<int>& deletedRemainingPtrs,
//		vector<int>& deletedStrsPtrs, const int threadIdx, const int threadNum, const int threadLoad,
//		const int maxCodeLen) {
//	int threadStart = threadIdx * threadLoad;
//	int threadEnd = threadStart + threadLoad;
//	if (threadIdx == threadNum - 1) {
//		threadEnd = remainingStrsPtrs.size();
//	}
//	int strPtri = remainingStrsPtrs[minSumPtr];
//	for (int remj = threadStart; remj < threadEnd; remj++) {
//		int strPtrj = remainingStrsPtrs[remj];
//		if (strPtri == strPtrj) {
//			deletedRemainingPtrs.push_back(remj);
//			deletedStrsPtrs.push_back(strPtrj);
//			continue;
//		}
//		bool distIsAtLeastMin = FastEditDistance0123(candidates[strPtri], candidates[strPtrj], minED,
//				cand0123Cont[strPtri], cand0123Cont[strPtrj]);
//		if (!distIsAtLeastMin) {
//			deletedRemainingPtrs.push_back(remj);
//			deletedStrsPtrs.push_back(strPtrj);
//		}
//	}
//}
//
//void InBallOfMinTHW(const vector<string>& candidates, const vector<vector<char> >& cand0123Cont, const int minED,
//		const int minSumPtr, const vector<int>& remainingStrsPtrs, vector<int>& deletedRemainingPtrs,
//		vector<int>& deletedStrsPtrs, const int maxCodeLen, const int threadNum) {
//	vector<vector<int> > threadDeletedRemainingPtrs(threadNum);
//	vector<vector<int> > threadDeletedStrsPtrs(threadNum);
//	vector<thread> threads;
//	int threadLoad = remainingStrsPtrs.size() / threadNum;
//	for (int i = 0; i < threadNum; i++) {
//		threads.push_back(
//				thread(InBallOfMin, ref(candidates), ref(cand0123Cont), minED, minSumPtr, ref(remainingStrsPtrs),
//						ref(threadDeletedRemainingPtrs[i]), ref(threadDeletedStrsPtrs[i]), i, threadNum, threadLoad,
//						maxCodeLen));
//	}
//	for (thread& th : threads)
//		th.join();
//
//	// merge partial thread vectors
//	for (int i = 0; i < threadNum; i++) {
//		deletedRemainingPtrs.insert(deletedRemainingPtrs.end(), threadDeletedRemainingPtrs[i].begin(),
//				threadDeletedRemainingPtrs[i].end());
//		deletedStrsPtrs.insert(deletedStrsPtrs.end(), threadDeletedStrsPtrs[i].begin(), threadDeletedStrsPtrs[i].end());
//	}
//}
//
//void SubColsONMEM(const vector<string>& strs, const vector<vector<char> >& cand0123Cont, const int minED,
//		const vector<int>& remainingStrsPtrs, vector<int>& sumRows, const vector<int>& deletedStrsPtrs,
//		const int threadIdx, const int threadNum, const int maxCodeLen) {
//	for (int remi = threadIdx; remi < (int) remainingStrsPtrs.size(); remi += threadNum) {
//		if (sumRows[remi] == 0)
//			continue;
//		int strPtri = remainingStrsPtrs[remi];
//		for (int strPtrj : deletedStrsPtrs) {
//			bool distIsAtLeastMin = FastEditDistance0123(strs[strPtri], strs[strPtrj], minED, cand0123Cont[strPtri],
//					cand0123Cont[strPtrj]);
//			if (!distIsAtLeastMin) {
//				sumRows[remi]--;
//				if (sumRows[remi] == 0)
//					break;
//			}
//		}
//	}
//}
//
//void SubColsONMEMTHW(const vector<string>& candidates, const vector<vector<char> >& cand0123Cont, const int minED,
//		const vector<int>& remainingStrsPtrs, vector<int>& sumRows, const vector<int>& deletedStrsPtrs,
//		const int maxCodeLen, const int threadNum) {
//	vector<thread> threads;
//	for (int i = 0; i < threadNum; i++) {
//		threads.push_back(
//				thread(SubColsONMEM, ref(candidates), ref(cand0123Cont), minED, ref(remainingStrsPtrs), ref(sumRows),
//						ref(deletedStrsPtrs), i, threadNum, maxCodeLen));
//	}
//	for (thread& th : threads)
//		th.join();
//}
//
//void SaveProgress(const vector<int>& remainingStrsPtrs, const vector<int>& sumRows, const vector<int>& chosenStrIdxs) {
//	IntVecToFile(remainingStrsPtrs, "progress_remaining.txt");
//	IntVecToFile(sumRows, "progress_sum_rows.txt");
//	IntVecToFile(chosenStrIdxs, "progress_chosen.txt");
//}
//
//void DelProgress() {
//	remove("progress_remaining.txt");
//	remove("progress_sum_rows.txt");
//	remove("progress_chosen.txt");
//}
//
//void ChooseMinSumRows(const vector<string>& candidates, const vector<vector<char> >& cand0123Cont, const int minED,
//		const int saveInterval, vector<int>& sumRows, vector<int>& remainingStrsPtrs, vector<int>& chosenStrsIdxs,
//		const int maxCodeLen, const int threadNum) {
//	clock_t lastSaveTime = clock();
//	while (not sumRows.empty()) {
//		int minSumPtr, minSumVal;
//		MinSumRowONMEMTHW(sumRows, minSumPtr, minSumVal, threadNum);
//		chosenStrsIdxs.push_back(remainingStrsPtrs[minSumPtr]);
//		if (minSumVal == 0) {
//			sumRows.erase(sumRows.begin() + minSumPtr);
//			remainingStrsPtrs.erase(remainingStrsPtrs.begin() + minSumPtr);
//			continue;
//		}
//		vector<int> deletedRemainingPtrs, deletedStrsPtrs;
//		InBallOfMinTHW(candidates, cand0123Cont, minED, minSumPtr, remainingStrsPtrs, deletedRemainingPtrs,
//				deletedStrsPtrs, maxCodeLen, threadNum);
//		for (vector<int>::reverse_iterator it = deletedRemainingPtrs.rbegin(); it != deletedRemainingPtrs.rend();
//				it++) {
//			int idx = *it;
//			sumRows.erase(sumRows.begin() + idx);
//			remainingStrsPtrs.erase(remainingStrsPtrs.begin() + idx);
//		}
//		SubColsONMEMTHW(candidates, cand0123Cont, minED, remainingStrsPtrs, sumRows, deletedStrsPtrs, maxCodeLen,
//				threadNum);
//		clock_t currentTime = clock();
//		int secs_since_last_save = double(currentTime - lastSaveTime) / CLOCKS_PER_SEC;
//		if (secs_since_last_save > saveInterval) {
//			SaveProgress(remainingStrsPtrs, sumRows, chosenStrsIdxs);
//			lastSaveTime = currentTime;
//			cout << "PROGRESS: Remaining strings num:\t" << remainingStrsPtrs.size() << endl;
//		}
//	}
//	DelProgress();
//}
//
//long long int SumVec(const vector<int>& vec) {
//	long long int sum = 0;
//	for (int num : vec)
//		sum += num;
//	return sum;
//}
//
//void FilterByDistMinONMEM(const vector<string>& candidates, vector<string>& chosenStrs, const Params& params,
//		long long int& matrixOnesNum) {
//	vector<int> chosenStrsIdxs;
//	int n = candidates.size();
//	vector<int> remainingStrsPtrs;
//	for (int i = 0; i < n; i++) {
//		remainingStrsPtrs.push_back(i);
//	}
//	vector<vector<char> > cand0123Cont = Cont0123(candidates);
//	NumToFile(1, "progress_stage.txt");
//
//	clock_t starts = clock();
//	vector<int> sumRows = SumRowsONMEM(candidates, cand0123Cont, params.minED, params.saveInterval, params.maxCodeLen,
//			params.threadNum);
//	clock_t ends = clock();
//	double elapsed_secss = double(ends - starts) / CLOCKS_PER_SEC;
//	cout << "SumRows Time:\t\t" << fixed << setprecision(2) << elapsed_secss << "\tseconds" << endl;
//
//	matrixOnesNum = SumVec(sumRows);
//	NumToFile(2, "progress_stage.txt");
//
//	clock_t startc = clock();
//	ChooseMinSumRows(candidates, cand0123Cont, params.minED, params.saveInterval, sumRows, remainingStrsPtrs,
//			chosenStrsIdxs, params.maxCodeLen, params.threadNum);
//	clock_t endc = clock();
//	double elapsed_secsc = double(endc - startc) / CLOCKS_PER_SEC;
//	cout << "ChooseMinSumRows Time:\t" << fixed << setprecision(2) << elapsed_secsc << "\tseconds" << endl;
//
//	for (int idx : chosenStrsIdxs) {
//		chosenStrs.push_back(candidates[idx]);
//	}
//	remove("progress_stage.txt");
//}
//
//// Create a set of codeLen length strings over {0,1,2,3} with pairwise minimum distance of minDist
//// Min algorithm:
//// C - a set of candidate strings of length codeLen.
//// create matrix M: M(i,j) = 1 if dist(C(i), C(j)) < minDist. Otherwise, 0.
//// G <- empty
//// while C is not empty:
////		k an index of a row of M with minimal sum
////		G <- C(k)
////		delete candidate C(k) and the strings in its minDist ball(C(l) s.t. dist(C(k),C(l)) < minDist).
////		delete rows and columns of M corresponding to the ball of C(k).
//// return G.
//
//// candidate strings: strings with min Hamming distance of 3 by Hamming codes. Filter by GCR.
//
//void GenerateCodebookMin(const Params& params) {
//	try {
//		ParamsToFile(params, "progress_params.txt");
//		PrintTestParams(params);
//
//// generate candidate strings
//		vector<string> filteredHamming = Candidates(params);
//		StrVecToFile(filteredHamming, "progress_cand.txt");
//
//// filter by edit distance
//		vector<string> codebook;
//		int candidateNum = filteredHamming.size();
//		long long int matrixOnesNum;
//
//		FilterByDistMinONMEM(filteredHamming, codebook, params, matrixOnesNum);
//
//		PrintTestResults(candidateNum, matrixOnesNum, codebook.size());
//
//		ToFile(codebook, params, candidateNum, matrixOnesNum);
//
////		VerifyDist(codebook, params.minED, params.maxCodeLen, params.threadNum);
//		cout << "=====================================================" << endl;
//		remove("progress_params.txt");
//		remove("progress_cand.txt");
//	} catch (const ofstream::failure& e) {
//		cout << "Can't write one or more progress files or a writing error occurred! Aborted." << endl;
//	}
//}
//
//void LoadProgressSumRows(vector<int>& threadSumRows, int& i, int threadIdx) {
//	string smFilename = "progress_sum_rows_" + to_string(threadIdx) + ".txt";
//	FileToIntVec(threadSumRows, smFilename);
//	string iFilename = "progress_sum_rows_i_" + to_string(threadIdx) + ".txt";
//	FileToNum(i, iFilename);
//}
//
//vector<int> SumRowsONMEMResumeFromFile(const vector<string>& candidates, const vector<vector<char> >& cand0123Cont,
//		const int minED, const int saveInterval, const int maxCodeLen, const int threadNum) {
//	vector<vector<int>> threadSumRows(threadNum);
//	vector<int> threadLastI(threadNum);
//	for (int i = 0; i < threadNum; i++) {
//		LoadProgressSumRows(threadSumRows[i], threadLastI[i], i);
//	}
//	vector<clock_t> threadLastSave(threadNum);
//	vector<thread> threads;
//	for (int i = 0; i < threadNum; i++) {
//		threads.push_back(
//				thread(SumRowSave, ref(candidates), ref(cand0123Cont), ref(threadSumRows[i]), minED, saveInterval,
//						ref(threadLastSave[i]), threadLastI[i] + threadNum, i, threadNum, maxCodeLen));
//	}
//	for (thread& th : threads)
//		th.join();
//
//	vector<int> sumRows(candidates.size());
//	for (int i = 0; i < threadNum; i++) {
//		for (int j = 0; j < (int) candidates.size(); j++) {
//			sumRows[j] += threadSumRows[i][j];
//		}
//	}
//	return sumRows;
//}
//
//void FilterByDistMinONMEMResumeFromFile(const vector<string>& candidates, vector<string>& chosenStrs,
//		const Params& params, long long int& matrixOnesNum) {
//	int stage;
//	vector<int> chosenStrsIdxs, remainingStrsPtrs, sumRows;
//	vector<vector<char> > cand0123Cont = Cont0123(candidates);
//	FileToNum(stage, "progress_stage.txt");
//	if (stage == 1) {
//		cout << "Resuming sum rows" << endl;
//		sumRows = SumRowsONMEMResumeFromFile(candidates, cand0123Cont, params.minED, params.saveInterval,
//				params.maxCodeLen, params.threadNum);
//		matrixOnesNum = SumVec(sumRows);
//		NumToFile(2, "progress_stage.txt");
//		int n = candidates.size();
//		for (int i = 0; i < n; i++) {
//			remainingStrsPtrs.push_back(i);
//		}
//	}
//	else {
//		assert(stage == 2);
//		cout << "Resuming min sum rows" << endl;
//		FileToIntVec(remainingStrsPtrs, "progress_remaining.txt");
//		cout << "remaining candidates:\t\t" << remainingStrsPtrs.size() << endl;
//		FileToIntVec(sumRows, "progress_sum_rows.txt");
//		FileToIntVec(chosenStrsIdxs, "progress_chosen.txt");
//	}
//
//	ChooseMinSumRows(candidates, cand0123Cont, params.minED, params.saveInterval, sumRows, remainingStrsPtrs,
//			chosenStrsIdxs, params.maxCodeLen, params.threadNum);
//	for (int idx : chosenStrsIdxs) {
//		chosenStrs.push_back(candidates[idx]);
//	}
//	remove("progress_stage.txt");
//}
//
//// If execution of GenerateCodebookMin was not completed, resume from file.
//
//void GenerateCodebookMinResumeFromFile() {
//	try {
//		Params params;
//		FileToParams(params, "progress_params.txt");
//		cout << "Resuming from file" << endl;
//		PrintTestParams(params);
//		vector<string> candidates;
//		FileToStrVec(candidates, "progress_cand.txt");
//		cout << "number of candidate strings:\t" << candidates.size() << endl;
//		vector<string> codebook;
//		int candidateNum = candidates.size();
//		long long int matrixOnesNum;
//		FilterByDistMinONMEMResumeFromFile(candidates, codebook, params, matrixOnesNum);
//		PrintTestResults(candidateNum, matrixOnesNum, codebook.size());
//		ToFile(codebook, params, candidateNum, matrixOnesNum);
////		VerifyDist(codebook, params.minED, params.maxCodeLen, params.threadNum);
//		cout << "=====================================================" << endl;
//		remove("progress_params.txt");
//		remove("progress_cand.txt");
//	} catch (const ifstream::failure& e) {
//		cout << "Read/Write progress files error! Aborted." << endl;
//	}
//}

//	void DelRowCol(int rc) {
//		unordered_map<int, unordered_set<int> >::iterator rowIt = m.find(rc);
//		int i = rowIt->first;
//		unordered_set<int>& js = rowIt->second;
//		// delete col for each j in js delete (j,i)
//		for (int j : js) {
//			m[j].erase(i);
//		}
//		// delete row
//		m.erase(rowIt);
//	}

	// return number of the row with minimum sum
//	const pair<const int, unordered_set<int> >* MinSumRow() const {
//		int minVal = INT_MAX;
//		const pair<const int, unordered_set<int> >* minEntryPtr = NULL;
//		for (const pair<const int, unordered_set<int> >& intStPr : m) {
//			int sumRow = intStPr.second.size();
//			if (sumRow < minVal) {
//				minVal = sumRow;
//				minEntryPtr = &intStPr;
//				if (minVal <= 1)
//					break;
//			}
//		}
//		assert(minEntryPtr!=NULL);
//		return minEntryPtr;
//	}

//	void DelBall(const pair<const int, unordered_set<int> >* matRow, unordered_set<int>& remaining) {
//		vector<int> toDel(matRow->second.begin(), matRow->second.end());
//		toDel.push_back(matRow->first);
//		for (int num : toDel) {
//			DelRowCol(num);
//			remaining.erase(num);
//		}
//	}


	// return entry with min sum and delete its ball
//	int FindMinDel(unordered_set<int>& remaining, clock_t& minSumRowTime, clock_t& delBallTime) {
//		clock_t msr_start = clock();
//		const pair<const int, unordered_set<int> >* minSumRow = MinSumRow();
//		clock_t msr_end = clock();
//		minSumRowTime += (msr_end - msr_start);
//
//		int minEntry = minSumRow->first;
//		clock_t db_start = clock();
//		DelBall(minSumRow, remaining);
//		clock_t db_end = clock();
//		delBallTime += (db_end - db_start);
//		return minEntry;
//	}

//void Codebook2(AdjList& adjList, vector<string>& codebook, const vector<string>& candidates, const int saveInterval,
//		const bool resume) {
//	codebook.clear();
//	clock_t lastSaveTime = clock();
//
//	unordered_set<int> remaining;
//	if (not resume) {
//		IndicesToSet(remaining, candidates.size());
//		SaveProgressCodebook(remaining, adjList, codebook);
//	}
//	else {
//		LoadProgressCodebook(remaining, adjList, codebook);
//	}
//	clock_t minSumRowTime = 0, delBallTime = 0;
//	while (not adjList.empty()) {
//		int minEntry = adjList.FindMinDel2(remaining, minSumRowTime, delBallTime);
//		codebook.push_back(candidates[minEntry]);
//		clock_t currentTime = clock();
//		int secs_since_last_save = double(currentTime - lastSaveTime) / CLOCKS_PER_SEC;
//		if (secs_since_last_save > saveInterval) {
//			SaveProgressCodebook(remaining, adjList, codebook);
//			lastSaveTime = currentTime;
//			cout << "Codebook PROGRESS: Remaining Rows " << adjList.RowNum() << endl;
//		}
//	}
//
//	double elapsed_secs_msr = double(minSumRowTime) / CLOCKS_PER_SEC;
//	cout << "Find Min Sum Row Time:\t" << fixed << setprecision(2) << elapsed_secs_msr << "\tseconds" << endl;
//	double elapsed_secs_db = double(delBallTime) / CLOCKS_PER_SEC;
//	cout << "Del Ball Time:\t\t" << fixed << setprecision(2) << elapsed_secs_db << "\tseconds" << endl;
//
//	for (int num : remaining) {
//		codebook.push_back(candidates[num]);
//	}
//	DelProgressCodebook();
//}

//void CodebookAdjList2(const vector<string>& candidates, vector<string>& codebook, const int minED, const int threadNum,
//		const int saveInterval, long long int& matrixOnesNum) {
//	AdjList adjList;
//	NumToFile(1, "progress_stage.txt");
//
//	clock_t starta = clock();
//	FillAdjList2(adjList, candidates, minED, threadNum, saveInterval, false, matrixOnesNum);
//	clock_t enda = clock();
//	double elapsed_secsa = double(enda - starta) / CLOCKS_PER_SEC;
//	cout << "Fill AdjList Time:\t" << fixed << setprecision(2) << elapsed_secsa << "\tseconds" << endl;
//
//	NumToFile(2, "progress_stage.txt");
//	LongLongIntToFile(matrixOnesNum, "matrix_ones_num.txt");
//
//	clock_t startc = clock();
//	Codebook2(adjList, codebook, candidates, saveInterval, false);
//	clock_t endc = clock();
//	double elapsed_secs = double(endc - startc) / CLOCKS_PER_SEC;
//	cout << "Process Matrix Time:\t" << fixed << setprecision(2) << elapsed_secs << "\tseconds" << endl;
//
//	remove("progress_stage.txt");
//	remove("matrix_ones_num.txt");
//}

//void GenerateCodebookAdj2(const Params& params) {
//	clock_t start = clock();
//	ParamsToFile(params, "progress_params.txt");
//	PrintTestParams(params);
//
//	vector<string> candidates = Candidates(params);
//	StrVecToFile(candidates, "progress_cand.txt");
//	vector<string> codebook;
//	long long int matrixOnesNum;
//
//	CodebookAdjList2(candidates, codebook, params.codeMinED, params.threadNum, params.saveInterval, matrixOnesNum);
//
//	long long int candidateNum = candidates.size();
//	PrintTestResults(candidateNum, matrixOnesNum, codebook.size());
//	ToFile(codebook, params, candidateNum, matrixOnesNum);
//	VerifyDist(codebook, params.codeMinED, params.threadNum);
//	cout << "=====================================================" << endl;
//	remove("progress_params.txt");
//	remove("progress_cand.txt");
//
//	clock_t end = clock();
//	double elapsed_secs = double(end - start) / CLOCKS_PER_SEC;
//	cout << "Codebook Time: " << fixed << setprecision(2) << elapsed_secs << "\tseconds" << endl;
//	cout << "=====================================================" << endl;
//}

//void FillAdjList(AdjList& adjList, const vector<string>& candidates, const int minED, const int threadNum,
//		const int saveInterval, const bool resume, long long int& matrixOnesNum) {
//	vector<vector<pair<int, int> > > threadPairVecs(threadNum, vector<pair<int, int> >());
//	vector<thread> threads;
//	vector<int> threadStartCand(threadNum);
//	vector<vector<char> > cand0123Cont = Cont0123(candidates);
//
//	if (not resume) {
//		for (int i = 0; i < threadNum; i++) {
//			threadStartCand[i] = i;
//		}
//	}
//	else {
//		for (int i = 0; i < threadNum; i++) {
//			LoadProgressAdjListComp(threadStartCand[i], threadPairVecs[i], i);
//			threadStartCand[i] += threadNum;
//		}
//	}
//
//	for (int i = 0; i < threadNum; i++) {
//		threads.push_back(
//				thread(FillAdjListTH, ref(threadPairVecs[i]), ref(candidates), ref(cand0123Cont), minED,
//						threadStartCand[i], i, threadNum, saveInterval));
//	}
//	for (thread& th : threads)
//		th.join();
//	matrixOnesNum = 0;
//	for (vector<pair<int, int> >& thvec : threadPairVecs) {
//		matrixOnesNum += (thvec.size() * 2);
//		for (pair<int, int>& pr : thvec) {
//			adjList.Set(pr.first, pr.second);
//			adjList.Set(pr.second, pr.first);
//		}
//	}
//	for (int i = 0; i < threadNum; i++) {
//		DelProgressAdjListComp(i);
//	}
//}


