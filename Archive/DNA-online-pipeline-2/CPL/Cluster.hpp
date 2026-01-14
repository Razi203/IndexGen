#ifndef CLUSTER_HPP_
#define CLUSTER_HPP_

#include <string>
#include <vector>
#include <random>
using namespace std;

struct Cluster
{
	string original;
	vector<string> copies;

	// Default constructor
	Cluster() = default;

	// Constructor with parameters
	Cluster(const unsigned strandLength, const int clonesNum, const double delProb, const double insProb,
			const double subProb, mt19937 &generator);

	// Constructor with dummy parameter
	Cluster(const unsigned strandLength, const int clonesNum, const double delProb, const double insProb,
			const double subProb, mt19937 &generator, bool dummy);

	// Constructor with original and copies
	Cluster(const string &original, const vector<string> &copies);

	// Rule of 5: Add proper copy/move constructors and assignment operators
	// Copy constructor
	Cluster(const Cluster &other) = default;

	// Move constructor
	Cluster(Cluster &&other) noexcept = default;

	// Copy assignment operator
	Cluster &operator=(const Cluster &other) = default;

	// Move assignment operator
	Cluster &operator=(Cluster &&other) noexcept = default;

	// Destructor
	~Cluster() = default;
};

string MakeStrand(const unsigned length, mt19937 &generator);

#endif /* CLUSTER_HPP_ */