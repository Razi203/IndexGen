#include "../../include/EditDistance.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct Cluster
{
    int id;
    std::string centroid;
    std::vector<std::string> members;
    int radius = 0;
    std::string farthest_member;
};

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_prefix>" << std::endl;
        return 1;
    }
    std::string filename = argv[1];
    std::string output_prefix = argv[2];

    std::ifstream infile(filename);

    if (!infile.is_open())
    {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return 1;
    }

    // 1. Load all lines into memory
    std::cout << "Loading file..." << std::endl;
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(infile, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(line);
    }
    infile.close();
    std::cout << "Loaded " << lines.size() << " lines." << std::endl;

    // 2. Parse Clusters
    std::cout << "Parsing clusters..." << std::endl;
    std::vector<Cluster> clusters;
    std::string marker = "*************";
    std::vector<size_t> marker_indices;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (lines[i] == marker)
        {
            marker_indices.push_back(i);
        }
    }

    for (size_t k = 0; k < marker_indices.size(); ++k)
    {
        size_t m_idx = marker_indices[k];
        if (m_idx == 0)
            continue; // Should not happen based on file format

        Cluster c;
        c.id = (int)k;
        c.centroid = lines[m_idx - 1];

        size_t start_member = m_idx + 1;
        size_t end_member;

        if (k + 1 < marker_indices.size())
        {
            // End before the next centroid (which is at next_marker - 1)
            end_member = marker_indices[k + 1] - 1;
        }
        else
        {
            // Last cluster goes to end of file
            end_member = lines.size();
        }

        // Safety check
        if (end_member > start_member)
        {
            for (size_t j = start_member; j < end_member; ++j)
            {
                if (!lines[j].empty())
                {
                    c.members.push_back(lines[j]);
                }
            }
        }
        clusters.push_back(c);
    }

    std::cout << "Parsed " << clusters.size() << " clusters." << std::endl;

    // 3. Calculate Radius
    std::cout << "Calculating radii..." << std::endl;
    for (auto &c : clusters)
    {
        if (c.members.empty())
        {
            c.radius = 0;
            continue;
        }
        PatternHandle ph(c.centroid);
        int max_dist = 0;
        for (const auto &mem : c.members)
        {
            int d = EditDistanceExact(mem, ph);
            if (d > max_dist)
            {
                max_dist = d;
                c.farthest_member = mem;
            }
        }
        c.radius = max_dist;
    }

    // 4. Centroid Histograms & Intersections
    std::cout << "Calculating centroid distances and intersections..." << std::endl;
    std::map<int, int> histogram;
    struct Intersection
    {
        int id1;
        int id2;
        int dist;
        int r1;
        int r2;
    };
    std::vector<Intersection> intersections;

    long long total_pairs = 0;
    for (size_t i = 0; i < clusters.size(); ++i)
    {
        PatternHandle ph(clusters[i].centroid);
        for (size_t j = i + 1; j < clusters.size(); ++j)
        {
            int dist = EditDistanceExact(clusters[j].centroid, ph);
            histogram[dist]++;
            total_pairs++;

            // We export 'Close' pairs (dist <= r1 + r2 + 3) which effectively covers intersections too
            if (dist <= (clusters[i].radius + clusters[j].radius + 3))
            {
                intersections.push_back({clusters[i].id, clusters[j].id, dist, clusters[i].radius, clusters[j].radius});
            }
        }
    }

    // 5. Output Results to CSV
    std::cout << "Exporting results to CSV files..." << std::endl;

    // Clusters CSV
    std::ofstream clusters_file(output_prefix + "_clusters.csv");
    clusters_file << "ClusterID,MemberCount,Radius,Centroid,FarthestMember\n";
    for (const auto &c : clusters)
    {
        clusters_file << c.id << "," << c.members.size() << "," << c.radius << "," << c.centroid << ","
                      << c.farthest_member << "\n";
    }
    clusters_file.close();

    // Histogram CSV
    std::ofstream hist_file(output_prefix + "_histogram.csv");
    hist_file << "Distance,Count\n";
    for (const auto &[dist, count] : histogram)
    {
        hist_file << dist << "," << count << "\n";
    }
    hist_file.close();

    // Intersections/Close Pairs CSV
    std::ofstream inter_file(output_prefix + "_pairs.csv");
    inter_file << "ClusterID1,ClusterID2,Distance,Radius1,Radius2,SumRadii\n";
    for (const auto &inter : intersections)
    {
        inter_file << inter.id1 << "," << inter.id2 << "," << inter.dist << "," << inter.r1 << "," << inter.r2 << ","
                   << (inter.r1 + inter.r2) << "\n";
    }
    inter_file.close();

    std::cout << "Analysis and Export Complete." << std::endl;
    return 0;
}
