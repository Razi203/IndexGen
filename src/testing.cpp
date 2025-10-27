/**
 * @file testing.cpp
 * @brief A standalone testing program.
 */

#include "Candidates/LinearCodes.hpp"
#include "Utils.hpp"
#include <iomanip>
#include <iostream>

using namespace std;

/**
 * @brief Prints usage information for the testing program.
 */
void print_usage(const char *program_name)
{
    cout << "Usage: " << program_name << " <code_length> <min_hamming_distance>" << endl;
    cout << endl;
    cout << "Arguments:" << endl;
    cout << "  code_length           Length of the codewords (n)" << endl;
    cout << "  min_hamming_distance  Minimum Hamming distance (2-5)" << endl;
    cout << endl;
    cout << "Example:" << endl;
    cout << "  " << program_name << " 21 3" << endl;
}

int main(int argc, char *argv[])
{
    // Check command-line arguments
    if (argc != 3)
    {
        cerr << "Error: Incorrect number of arguments." << endl << endl;
        print_usage(argv[0]);
        return 1;
    }

    // Parse arguments
    int code_length = 0;
    int min_hamming_dist = 0;

    try
    {
        code_length = stoi(argv[1]);
        min_hamming_dist = stoi(argv[2]);
    }
    catch (const exception &e)
    {
        cerr << "Error: Invalid arguments. Both must be integers." << endl << endl;
        print_usage(argv[0]);
        return 1;
    }

    // Validate input
    if (code_length < 1)
    {
        cerr << "Error: Code length must be at least 1." << endl;
        return 1;
    }

    if (min_hamming_dist < 2 || min_hamming_dist > 5)
    {
        cerr << "Error: Minimum Hamming distance must be between 2 and 5." << endl;
        return 1;
    }

    // Additional validation based on code requirements
    int required_min_length = 0;
    switch (min_hamming_dist)
    {
    case 2:
        required_min_length = 2;
        break;
    case 3:
        required_min_length = 4;
        break;
    case 4:
        required_min_length = 6;
        break;
    case 5:
        required_min_length = 8;
        break;
    }

    if (code_length < required_min_length)
    {
        cerr << "Error: For Hamming distance " << min_hamming_dist << ", code length must be at least "
             << required_min_length << "." << endl;
        return 1;
    }

    // Generate the linear code
    cout << "==============================================\n";
    cout << "Linear Code Candidate Generation Test\n";
    cout << "==============================================\n";
    cout << "Parameters:\n";
    cout << "  Code Length (n):              " << code_length << endl;
    cout << "  Min Hamming Distance (d):     " << min_hamming_dist << endl;
    cout << "----------------------------------------------\n";

    cout << "Generating linear code candidates..." << endl;

    vector<vector<int>> coded_vecs = CodedVecs(code_length, min_hamming_dist);

    cout << "Generation complete!" << endl;
    cout << "----------------------------------------------\n";
    cout << "Results:\n";
    cout << "  Number of Candidates:         " << coded_vecs.size() << endl;
    cout << "==============================================\n";

    // // Optional: Display first few candidates if the set is not too large
    // if (coded_vecs.size() <= 20)
    // {
    //     cout << "\nFirst " << coded_vecs.size() << " candidates:\n";
    //     for (size_t i = 0; i < coded_vecs.size(); ++i)
    //     {
    //         cout << "  " << setw(3) << i + 1 << ": " << VecToStr(coded_vecs[i]) << endl;
    //     }
    // }
    // else if (coded_vecs.size() > 0)
    // {
    //     cout << "\nFirst 10 candidates (of " << coded_vecs.size() << "):\n";
    //     for (size_t i = 0; i < 10; ++i)
    //     {
    //         cout << "  " << setw(3) << i + 1 << ": " << VecToStr(coded_vecs[i]) << endl;
    //     }
    //     cout << "  ...\n";
    // }

    return 0;
}