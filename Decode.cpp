#include "Decode.hpp"
#include "Utils.hpp"
#include <fstream>
#include <iostream>
#include <cassert>
#include <unordered_set>
#include <thread>
#include <iomanip>

void CodebookFromFile(const string& filename, vector<string>& codebook) {
	ifstream input;
	input.open(filename.c_str());
	if (!input.is_open()) {
		cout << "Failed opening input file!" << endl;
		return;
	}
	string line;
	// Pass through meta data
	while (getline(input, line)) {
//		cout << line << endl;
		assert(not line.empty());
		if (line[0] == '=')
			break;
	}
	while (getline(input, line)) {
		codebook.push_back(line);
	}
	cout << "Read " << codebook.size() << " words from file" << endl;
	input.close();
}

string ClosestWord(const vector<string>& codebook, const vector<vector<char> >& cont0123Code,
		const string& receivedWord, const vector<char>& cont0123Rec, const int codeLen) {
	int codeSize = codebook.size();
	int minED = codeLen + 1;
	int closestCodewordIdx = -1;
	for (int i = 0; i < codeSize; i++) {
		int currED = FastEditDistance0123ForSearch(receivedWord, codebook[i], minED, cont0123Rec, cont0123Code[i]);
		if (currED != -1) { // ED(x,y) < minED
			minED = currED;
			closestCodewordIdx = i;
			assert(minED != 0);
			if (minED == 1) {
				break;
			}
		}
	}
	assert(closestCodewordIdx != -1);
	return codebook[closestCodewordIdx];
}

string NearestCodeword(const vector<string>& codebook, const vector<vector<char> >& cont0123Code,
		const unordered_set<string>& codebookSet, const string& receivedWord, const vector<char>& cont0123Rec,
		const int codeLen) {

	unordered_set<string>::const_iterator rIt = codebookSet.find(receivedWord);
	if (rIt != codebookSet.end())
		return receivedWord;
	else
		return ClosestWord(codebook, cont0123Code, receivedWord, cont0123Rec, codeLen);
}

void DecodeNoThreads(const vector<string>& codebook, const vector<string>& receivedWords, vector<string>& decodedWords,
		const int codeLen) {
	int receivedWordsNum = receivedWords.size();
	decodedWords = vector<string>(receivedWordsNum);
	vector<vector<char> > cont0123Code = Cont0123(codebook);
	vector<vector<char> > cont0123Received = Cont0123(receivedWords);
	unordered_set<string> codebookSet = unordered_set<string>(codebook.begin(), codebook.end());
	for (int i = 0; i < receivedWordsNum; i++) {
		decodedWords[i] = NearestCodeword(codebook, cont0123Code, codebookSet, receivedWords[i], cont0123Received[i],
				codeLen);
	}
}

void DecodeThread(const vector<string>& codebook, const vector<string>& receivedWords, vector<string>& decodedWords,
		const unordered_set<string>& codebookSet, const vector<vector<char> >& cont0123Code,
		const vector<vector<char> >& cont0123Received, const int codeLen, const int threadNum, const int threadId) {
	int receivedWordsNum = receivedWords.size();
	for (int i = threadId; i < receivedWordsNum; i += threadNum) {
		decodedWords[i] = NearestCodeword(codebook, cont0123Code, codebookSet, receivedWords[i], cont0123Received[i],
				codeLen);
	}
}

void DecodeThreads(const vector<string>& codebook, const vector<string>& receivedWords, vector<string>& decodedWords,
		const int codeLen, const int threadNum) {
	int receivedWordsNum = receivedWords.size();
	decodedWords = vector<string>(receivedWordsNum);
	vector<vector<char> > cont0123Code = Cont0123(codebook);
	vector<vector<char> > cont0123Received = Cont0123(receivedWords);
	unordered_set<string> codebookSet = unordered_set<string>(codebook.begin(), codebook.end());

	vector<thread> threads;
	for (int threadId = 0; threadId < threadNum; threadId++) {
		threads.push_back(
				thread(DecodeThread, ref(codebook), ref(receivedWords), ref(decodedWords), ref(codebookSet), ref(cont0123Code),
						ref(cont0123Received), codeLen, threadNum, threadId));
	}
	for (thread& th : threads)
		th.join();
}

void Decode(const vector<string>& codebook, const vector<string>& receivedWords, vector<string>& decodedWords,
		const int codeLen, const int threadNum) {
	if (threadNum == 1) {
		DecodeNoThreads(codebook, receivedWords, decodedWords, codeLen);
	}
	else {
		DecodeThreads(codebook, receivedWords, decodedWords, codeLen, threadNum);
	}
}

string ClosestCodewordBrute(const vector<string>& codebook, const string& receivedWord) {
	int codeSize = codebook.size();
	int minED = receivedWord.size() + 1;
	int minCodewordIdx = -1;
	for (int i = 0; i < codeSize; i++) {
		int currED = FastEditDistance(codebook[i], receivedWord);
		if (currED < minED) {
			minED = currED;
			minCodewordIdx = i;
		}
	}
	assert(minCodewordIdx != -1);
	return codebook[minCodewordIdx];
}

void RandomWords(const int wordNum, const int wordLen, vector<string>& words, mt19937& generator) {
	words = vector<string>(wordNum);
	for (int i = 0; i < wordNum; i++) {
		words[i] = MakeStrand0123(wordLen, generator);
	}
}

bool TestDecodedWords(const vector<string>& codebook, const vector<string>& receivedWords,
		const vector<string>& decodedWords) {
	int receivedWordsNum = receivedWords.size();
	for (int i = 0; i < receivedWordsNum; i++) {
		int edClosest = FastEditDistance(receivedWords[i], decodedWords[i]);
		string closestCodewordBrute = ClosestCodewordBrute(codebook, receivedWords[i]);
		int edClosestBrute = FastEditDistance(receivedWords[i], closestCodewordBrute);
		if (edClosestBrute < edClosest) {
			return false;
		}
	}
	return true;
}

void TestDecode(const int testNum, const int wordNum, const string& codebookFilename, const int threadNum) {
	unsigned sd = chrono::high_resolution_clock::now().time_since_epoch().count();
	mt19937 generator(sd);
	vector<string> codebook, receivedWords, decodedWords;
	CodebookFromFile(codebookFilename, codebook);
	assert(not codebook.empty());
	int codeLen = codebook[0].size();

	for (int i = 0; i < testNum; i++) {
		RandomWords(wordNum, codeLen, receivedWords, generator);
		Decode(codebook, receivedWords, decodedWords, codeLen, threadNum);
		bool result = TestDecodedWords(codebook, receivedWords, decodedWords);
		if (not result) {
			cout << "Test Decode Failure!" << endl;
			return;
		}
	}
	cout << "Test Decode Success!" << endl;
}

void TestDecodeTime1(const int wordNum, const string& codebookFilename) {
	unsigned sd = chrono::high_resolution_clock::now().time_since_epoch().count();
	mt19937 generator(sd);
	vector<string> codebook, receivedWords, decodedWords;
	CodebookFromFile(codebookFilename, codebook);
	assert(not codebook.empty());
	int codeLen = codebook[0].size();

	RandomWords(wordNum, codeLen, receivedWords, generator);

	clock_t begin = clock();
	int threadNum = 1;
	Decode(codebook, receivedWords, decodedWords, codeLen, threadNum);
	clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	cout << "Decode Time 1:\t" << fixed << setprecision(2) << elapsed_secs << "\tseconds" << endl;
}

void TestDecodeTime2(const int wordNum, const string& codebookFilename) {
	unsigned sd = chrono::high_resolution_clock::now().time_since_epoch().count();
	mt19937 generator(sd);
	vector<string> codebook, receivedWords, decodedWords;
	CodebookFromFile(codebookFilename, codebook);
	assert(not codebook.empty());
	int codeLen = codebook[0].size();

	RandomWords(wordNum, codeLen, receivedWords, generator);

	clock_t begin = clock();
	int threadNum = 2;
	Decode(codebook, receivedWords, decodedWords, codeLen, threadNum);
	clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	cout << "Decode Time 2:\t" << fixed << setprecision(2) << elapsed_secs << "\tseconds" << endl;
}
