#ifndef INDEXGEN_HPP_
#define INDEXGEN_HPP_

struct Params {
	// length of code words
	int codeLen;

	// minimal Hamming distance between any two candidates. from {1,2,3,4,5}
	int candMinHD;
	// minimal edit distance between any two code words. from {3,4,5}
	int codeMinED;

	// longest allowed homopolymer. for no restriction on homopolymers set maxRun=0
	int maxRun;
	// minimal GC content. for no restriction on GC content set minGCCont=0
	double minGCCont;
	// maximal GC content
	double maxGCCont;

	// number of threads on system
	int threadNum;
	// save interval between two save points in seconds
	int saveInterval;

	Params():codeLen(), candMinHD(), codeMinED(), maxRun(), minGCCont(), maxGCCont(), threadNum(), saveInterval(){};
	Params(const int codeLen, const int minHD, const int minED, const int maxRun,
			const double minGCCont, const double maxGCCont, const int threadNum, const int saveInterval) :
			codeLen(codeLen), candMinHD(minHD), codeMinED(minED), maxRun(maxRun), minGCCont(minGCCont), maxGCCont(
					maxGCCont), threadNum(threadNum), saveInterval(saveInterval) {

	}
};

#endif /* INDEXGEN_HPP_ */
