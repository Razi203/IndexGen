#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>
#include <fstream>
#include <map>
#include <algorithm>
#include "Cluster.hpp"
#include "GuessFunctions.hpp"
#include "EditDistance.hpp"
#include "Utils.hpp"
#include "DNAV8_amir.hpp"
using namespace std;

string reconstruct_cpl_imp(vector<string> &reads, int original_length)
{
    int maxCopies = 32;

    string original(original_length, 'A');
    int R = 1;
    unsigned sd = chrono::high_resolution_clock::now().time_since_epoch().count();
    mt19937 generator(sd);

    // Check input validity
    if (reads.empty())
    {
        cout << "Error: reads vector is empty" << endl;
        return "";
    }

    std::vector<std::string> copies;
    try
    {
        for (const std::string &read : reads)
        {
            if (read.empty())
            {
                cout << "Warning: empty read found, skipping" << endl;
                continue;
            }
            copies.push_back(read);
        }
    }
    catch (const std::exception &e)
    {
        cout << "Error copying reads: " << e.what() << endl;
        return "";
    }

    vector<string> copies2;
    copies2.clear();

    try
    {
        if (static_cast<int>(copies.size()) <= maxCopies)
        {
            copies2 = copies;
        }
        else
        {
            for (int i = 0; i < maxCopies && i < static_cast<int>(copies.size()); ++i)
            {
                copies2.push_back(copies[i]);
            }
        }
    }
    catch (const std::exception &e)
    {
        cout << "Error sampling copies: " << e.what() << endl;
        return "";
    }

    string finalGuess;

    if (copies2.size() == 1)
    {
        finalGuess = copies2[0];
    }
    else
    {
        try
        {
            // Create cluster in a more controlled way
            Cluster cluster(original, copies2);

            finalGuess = MinSumEDSimpleCorrectedClusterTwiceJoinR(cluster, original_length, generator, FixedLenMarkov, R);
        }
        catch (const std::exception &e)
        {
            cout << "Exception in cluster operations: " << e.what() << endl;
            return "";
        }
        catch (...)
        {
            cout << "Unknown exception in cluster operations" << endl;
            return "";
        }
    }
    return finalGuess;
}