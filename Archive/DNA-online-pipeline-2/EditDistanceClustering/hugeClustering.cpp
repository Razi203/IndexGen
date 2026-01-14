#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <climits>
#include <iostream>
#include <numeric>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <memory>

#include "../CPL/DNAV8_amir.hpp"
#include "../utils.hpp"
#include "HDEQED.hpp"

using namespace std;

// Configuration constants
const string CENTROID_TYPE = "HDEQEDMinSumOfCorrectedClusterFast"; // CPL or HDEQEDFixMinSumFast or HDEQEDMinSumOfCorrectedClusterFast
const bool IS_SMALL_TEST = false;
const int INDEX_LEN = IS_SMALL_TEST ? 14 : 20;
const string INDICES_FILE_PATH = IS_SMALL_TEST ? "indices_1000.txt" : "125m_dna_strings.txt";
const string file_name = "huge_clusters_125m_" + CENTROID_TYPE + "_iterNum=";
const string file_ext = ".txt";

// Forward declaration of StringKMeans class (assuming it's in a header or copied here)
class StringKMeans
{
private:
    int k;
    int max_iterations;
    std::vector<std::string> data;
    std::vector<int> assignments;
    std::vector<std::string> centroids;
    std::mt19937 rng;

public:
    StringKMeans(int k, int max_iter = 100, int seed = 42)
        : k(k), max_iterations(max_iter), rng(seed) {}

    std::string calculateCentroid_min_edit_distance(const std::vector<std::string> &cluster_strings)
    {
        if (cluster_strings.empty())
            return "";
        if (cluster_strings.size() == 1)
            return cluster_strings[0];

        std::string best_centroid = cluster_strings[0];
        int min_total_distance = INT_MAX;

        for (const auto &candidate : cluster_strings)
        {
            int total_distance = 0;
            for (const auto &str : cluster_strings)
            {
                total_distance += FastEditDistance(candidate, str);
            }
            if (total_distance < min_total_distance)
            {
                min_total_distance = total_distance;
                best_centroid = candidate;
            }
        }
        return best_centroid;
    }

    std::string calculateCentroid(std::vector<std::string> &cluster_strings)
    {
        std::string result;
        if (CENTROID_TYPE == "min_edit_distance")
        {
            result = calculateCentroid_min_edit_distance(cluster_strings);
        }
        else if (CENTROID_TYPE == "CPL")
        {
            result = reconstruct_cpl_imp(cluster_strings, INDEX_LEN);
        }
        else if (CENTROID_TYPE == "HDEQEDFixMinSumFast")
        {
            result = HDEQEDFixMinSumFast(cluster_strings, INDEX_LEN);
        }
        else if (CENTROID_TYPE == "HDEQEDMinSumOfCorrectedClusterFast")
        {
            result = HDEQEDMinSumOfCorrectedClusterFast(cluster_strings, INDEX_LEN);
        }
        else
        {
            cout << "Error: Unsupported CENTROID_TYPE! \n";
            result = "";
        }

        return result;
    }

    void initializeCentroids()
    {
        centroids.clear();
        centroids.reserve(k);

        std::vector<int> indices(data.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), rng);

        for (int i = 0; i < k && i < data.size(); ++i)
        {
            centroids.push_back(data[indices[i]]);
        }

        while (centroids.size() < k)
        {
            centroids.push_back(data[rng() % data.size()]);
        }
    }

    bool assignToClusters()
    {
        std::vector<int> new_assignments(data.size());
        bool changed = false;

        for (int i = 0; i < data.size(); ++i)
        {
            int best_cluster = 0;
            int min_distance = FastEditDistance(data[i], centroids[0]);

            for (int j = 1; j < k; ++j)
            {
                int distance = FastEditDistanceWithThresholdBanded(data[i], centroids[j], min_distance);
                if (distance < min_distance)
                {
                    min_distance = distance;
                    best_cluster = j;
                }
            }

            new_assignments[i] = best_cluster;
            if (i >= assignments.size() || assignments[i] != best_cluster)
            {
                changed = true;
            }
        }

        assignments = new_assignments;
        return changed;
    }

    void updateCentroids()
    {
        std::vector<std::vector<std::string>> clusters(k);
        for (int i = 0; i < data.size(); ++i)
        {
            clusters[assignments[i]].push_back(data[i]);
        }

        for (int i = 0; i < k; ++i)
        {
            if (!clusters[i].empty())
            {
                centroids[i] = calculateCentroid(clusters[i]);
            }
        }
    }

    std::vector<int> fit(const std::vector<std::string> &input_data)
    {
        data = input_data;
        assignments.clear();

        if (data.empty())
            return assignments;

        initializeCentroids();

        for (int iter = 0; iter < max_iterations; ++iter)
        {
            bool changed = assignToClusters();
            if (!changed)
            {
                std::cout << "Converged after " << iter + 1 << " iterations.\n";
                break;
            }
            updateCentroids();
        }

        return assignments;
    }

    std::vector<std::string> getCentroids() const { return centroids; }
    std::vector<int> getAssignments() const { return assignments; }
};

// Cluster container class
class Cluster
{
public:
    std::string centroid;
    std::vector<int> data_indices;

    Cluster(const std::string &cent, const std::vector<int> &indices)
        : centroid(cent), data_indices(indices) {}
};

// Main HierarchicalClusterer class
class HierarchicalClusterer
{
private:
    int mega_cluster_size;
    int centroids_per_mega;
    int final_clusters_num;
    int max_iterations;
    int seed;

    std::vector<std::string> original_data;
    std::vector<std::vector<std::string>> current_mega_clusters;
    std::mt19937 rng;

public:
    HierarchicalClusterer(int mega_size = 10000,
                          int centroids_per = 100,
                          int final_clusters = 100,
                          int max_iter = 100,
                          int random_seed = 42)
        : mega_cluster_size(mega_size),
          centroids_per_mega(centroids_per),
          final_clusters_num(final_clusters),
          max_iterations(max_iter),
          seed(random_seed),
          rng(random_seed) {}

    // Create initial random mega-clusters
    std::vector<std::vector<std::string>> createInitialMegaClusters(const std::vector<std::string> &data)
    {
        std::cout << "Creating initial mega-clusters from " << data.size() << " items..." << std::endl;

        // Shuffle data indices
        std::vector<int> shuffled_indices(data.size());
        std::iota(shuffled_indices.begin(), shuffled_indices.end(), 0);
        std::shuffle(shuffled_indices.begin(), shuffled_indices.end(), rng);

        int n_mega_clusters = (data.size() + mega_cluster_size - 1) / mega_cluster_size;
        std::vector<std::vector<std::string>> mega_clusters(n_mega_clusters);

        for (int i = 0; i < data.size(); ++i)
        {
            int mega_idx = i / mega_cluster_size;
            mega_clusters[mega_idx].push_back(data[shuffled_indices[i]]);
        }

        std::cout << "Created " << mega_clusters.size() << " mega-clusters" << std::endl;
        return mega_clusters;
    }

    // Cluster a single mega-cluster
    std::pair<std::vector<std::string>, std::vector<int>> clusterMegaCluster(const std::vector<std::string> &mega_cluster_data)
    {
        // Remove duplicates while preserving mapping to original indices
        std::unordered_map<std::string, std::vector<int>> unique_data_map;
        for (int i = 0; i < mega_cluster_data.size(); ++i)
        {
            unique_data_map[mega_cluster_data[i]].push_back(i);
        }

        std::vector<std::string> unique_data;
        unique_data.reserve(unique_data_map.size());
        for (const auto &entry : unique_data_map)
        {
            unique_data.push_back(entry.first);
        }

        if (unique_data.size() < centroids_per_mega)
        {
            std::cerr << "Not enough unique data points for clustering (found "
                      << unique_data.size() << ", needed " << centroids_per_mega << ")" << std::endl;
            exit(1);
        }

        // Create and fit clusterer
        StringKMeans clusterer(centroids_per_mega, max_iterations, seed);
        std::vector<int> unique_labels = clusterer.fit(unique_data);

        // Get centroids
        std::vector<std::string> centroids = clusterer.getCentroids();

        // Create mapping from unique string to cluster label
        std::unordered_map<std::string, int> unique_string_to_label;
        for (size_t i = 0; i < unique_data.size(); ++i)
        {
            unique_string_to_label[unique_data[i]] = unique_labels[i];
        }

        // Assign labels to all original data using the mapping
        std::vector<int> labels(mega_cluster_data.size());
        for (const auto &entry : unique_data_map)
        {
            const std::string &s = entry.first;
            int cluster_label = unique_string_to_label[s];
            for (int index : entry.second)
            {
                labels[index] = cluster_label;
            }
        }

        return std::make_pair(centroids, labels);
    }

    // Cluster all mega-clusters
    std::pair<std::vector<std::vector<std::string>>, std::vector<std::vector<int>>>
    clusterAllMegaClusters(const std::vector<std::vector<std::string>> &mega_clusters)
    {
        std::cout << "Clustering " << mega_clusters.size() << " mega-clusters..." << std::endl;

        std::vector<std::vector<std::string>> all_centroids;
        std::vector<std::vector<int>> all_labels;

        for (int i = 0; i < mega_clusters.size(); ++i)
        {
            if (i % 10 == 0)
            {
                std::cout << "  Processing mega-cluster " << (i + 1) << "/" << mega_clusters.size() << std::endl;
            }

            auto [centroids, labels] = clusterMegaCluster(mega_clusters[i]);
            all_centroids.push_back(centroids);
            all_labels.push_back(labels);
        }

        return std::make_pair(all_centroids, all_labels);
    }

    // Cluster all centroids to reorganize
    std::pair<std::vector<std::string>, std::vector<std::vector<std::pair<int, int>>>>
    clusterCentroids(const std::vector<std::vector<std::string>> &all_centroids)
    {
        std::cout << "Clustering all centroids..." << std::endl;

        // Flatten all centroids while preserving mapping to original indices
        std::vector<std::string> flat_centroids;
        std::vector<std::pair<int, int>> index_mapping; // Maps flat index to (mega_idx, local_idx)

        for (int mega_idx = 0; mega_idx < all_centroids.size(); ++mega_idx)
        {
            for (int local_idx = 0; local_idx < all_centroids[mega_idx].size(); ++local_idx)
            {
                flat_centroids.push_back(all_centroids[mega_idx][local_idx]);
                index_mapping.push_back({mega_idx, local_idx});
            }
        }

        std::cout << "  Total centroids to cluster: " << flat_centroids.size() << std::endl;

        // Remove duplicates while preserving mapping
        std::unordered_map<std::string, std::vector<int>> unique_centroids_map;
        for (int i = 0; i < flat_centroids.size(); ++i)
        {
            unique_centroids_map[flat_centroids[i]].push_back(i);
        }

        std::vector<std::string> unique_centroids;
        unique_centroids.reserve(unique_centroids_map.size());
        for (const auto &entry : unique_centroids_map)
        {
            unique_centroids.push_back(entry.first);
        }

        // Cluster the unique centroids
        StringKMeans centroid_clusterer(final_clusters_num, max_iterations, seed);
        std::vector<int> unique_labels = centroid_clusterer.fit(unique_centroids);
        std::vector<std::string> final_centroids_vec = centroid_clusterer.getCentroids();

        // Create mapping from unique centroid to cluster label
        std::unordered_map<std::string, int> centroid_to_cluster;
        for (size_t i = 0; i < unique_centroids.size(); ++i)
        {
            centroid_to_cluster[unique_centroids[i]] = unique_labels[i];
        }

        // Assign all centroids using the mapping
        std::vector<int> centroid_labels(flat_centroids.size());
        for (const auto &entry : unique_centroids_map)
        {
            const std::string &centroid = entry.first;
            int cluster_label = centroid_to_cluster[centroid];
            for (int flat_idx : entry.second)
            {
                centroid_labels[flat_idx] = cluster_label;
            }
        }

        // Organize assignments
        std::vector<std::vector<std::pair<int, int>>> assignments(final_clusters_num);
        for (int flat_idx = 0; flat_idx < flat_centroids.size(); ++flat_idx)
        {
            int cluster_id = centroid_labels[flat_idx];
            assignments[cluster_id].push_back(index_mapping[flat_idx]);
        }

        return std::make_pair(final_centroids_vec, assignments);
    }

    // Reorganize data based on centroid clustering
    std::vector<std::vector<std::string>> reorganizeData(
        const std::vector<std::vector<std::string>> &mega_clusters,
        const std::vector<std::vector<int>> &all_labels,
        const std::vector<std::vector<std::pair<int, int>>> &assignments)
    {

        // all_labels[i][j] has the number of centroid to which the jth data point in the ith mega cluster belongs
        // assignments[i] holds the list of (mega_cluster_index, local_centroid_index) pairs assigned to the cluster of final_centroids_vec[i]

        std::cout << "Reorganizing data into new mega-clusters..." << std::endl;

        std::vector<std::vector<std::string>> new_mega_clusters(final_clusters_num);

        // For each new cluster, collect all data points assigned to its centroids
        for (int new_cluster_id = 0; new_cluster_id < assignments.size(); ++new_cluster_id)
        {
            for (const auto &[mega_idx, local_centroid_idx] : assignments[new_cluster_id])
            {
                // Find all data points in this mega-cluster assigned to this centroid
                const auto &mega_cluster_data = mega_clusters[mega_idx];
                const auto &mega_cluster_labels = all_labels[mega_idx];

                for (int i = 0; i < mega_cluster_data.size(); ++i)
                {
                    if (mega_cluster_labels[i] == local_centroid_idx)
                    {
                        new_mega_clusters[new_cluster_id].push_back(mega_cluster_data[i]);
                    }
                }
            }
        }

        std::cout << "Created " << new_mega_clusters.size() << " new mega-clusters" << std::endl;
        return new_mega_clusters;
    }

    void save_mega_clusters_to_file(std::vector<std::string> &final_centroids_vec, int iter_num)
    {
        // put clusters in file in the following structure:
        // line with centroid, then line with *************, then all of the strings in the cluster, then an empty line, then a new cluster..
        // the centroid of ith cluser is final_centroids_vec[i], and the strings are in current_mega_clusters[i]
        std::string filename = file_name + std::to_string(iter_num) + "_mega_clusters" + file_ext;
        std::ofstream outfile(filename);
        if (!outfile)
        {
            std::cerr << "Error: Cannot open file " << filename << " for writing." << std::endl;
            return;
        }
        for (int i = 0; i < current_mega_clusters.size(); ++i)
        {
            outfile << final_centroids_vec[i] << "\n";
            outfile << "*************\n";
            for (const auto &s : current_mega_clusters[i])
            {
                outfile << s << "\n";
            }
            outfile << "\n"; // Empty line between clusters
        }
        outfile.close();
        std::cout << "Saved mega-clusters to " << filename << std::endl;
    }

    void save_small_clusters_to_file(const std::vector<std::vector<std::string>> &all_centroids,
                                     const std::vector<std::vector<int>> &all_labels, int iter_num)
    {
        // put clusters in file in the following structure:
        // line with centroid, then line with *************, then all of the strings in the cluster, then an empty line, then a new cluster..
        // the centroid of ith cluser is all_centroids[mega_cluster_index][centroid_index], and the strings are in current_mega_clusters[mega_cluster_index] that have label centroid_index

        // all_centroids[i] has the centroids in the ith mega cluster
        // all_labels[i][j] has the number of centroid (index in all_centroids[i]) to which the jth data point in the ith mega cluster belongs

        std::string filename = file_name + std::to_string(iter_num) + "_small_clusters" + file_ext;
        std::ofstream outfile(filename);
        if (!outfile)
        {
            std::cerr << "Error: Cannot open file " << filename << " for writing." << std::endl;
            return;
        }
        for (int mega_idx = 0; mega_idx < all_centroids.size(); ++mega_idx)
        {
            for (int centroid_idx = 0; centroid_idx < all_centroids[mega_idx].size(); ++centroid_idx)
            {
                outfile << all_centroids[mega_idx][centroid_idx] << "\n";
                outfile << "*************\n";
                for (int i = 0; i < all_labels[mega_idx].size(); ++i)
                {
                    if (all_labels[mega_idx][i] == centroid_idx)
                    {
                        outfile << current_mega_clusters[mega_idx][i] << "\n";
                    }
                }
                outfile << "\n"; // Empty line between clusters
            }
        }
        outfile.close();
        std::cout << "Saved small clusters to " << filename << std::endl;
    }

public:
    // Main fit function
    void fit(const std::vector<std::string> &data, int n_iterations = 5, bool verbose = true, std::vector<int> itrs_to_save_on = {})
    {
        std::cout << "Starting hierarchical clustering with " << data.size() << " items..." << std::endl;
        std::cout << "Configuration: " << n_iterations << " iterations, "
                  << mega_cluster_size << " items per mega-cluster" << std::endl;

        original_data = data;
        current_mega_clusters = createInitialMegaClusters(data);

        for (int iteration = 0; iteration < n_iterations; ++iteration)
        {
            if (verbose)
            {
                std::cout << "\n=== Iteration " << (iteration + 1) << "/" << n_iterations << " ===" << std::endl;
            }

            auto start_time = std::chrono::high_resolution_clock::now();

            // Step 1: Cluster each mega-cluster
            auto [all_centroids, all_labels] = clusterAllMegaClusters(current_mega_clusters);

            // all_centroids[i] has the centroids in the ith mega cluster
            // all_labels[i][j] has the number of centroid (index in all_centroids[i]) to which the jth data point in the ith mega cluster belongs

            if (std::find(itrs_to_save_on.begin(), itrs_to_save_on.end(), iteration + 1) != itrs_to_save_on.end())
            {
                save_small_clusters_to_file(all_centroids, all_labels, iteration + 1);
            }

            // Step 2: Cluster all centroids to reorganize
            auto [final_centroids_vec, assignments] = clusterCentroids(all_centroids);

            // final_centroids_vec is the centroids in the clustering of the centroids
            // assignments[i] holds the list of (mega_cluster_index, local_centroid_index) pairs assigned to the cluster of final_centroids_vec[i]

            current_mega_clusters = reorganizeData(current_mega_clusters, all_labels, assignments);

            if (std::find(itrs_to_save_on.begin(), itrs_to_save_on.end(), iteration + 1) != itrs_to_save_on.end())
            {
                save_mega_clusters_to_file(final_centroids_vec, iteration + 1);
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = end_time - start_time;
            if (verbose)
            {
                std::cout << "Iteration " << (iteration + 1) << " completed in "
                          << duration.count() << " seconds" << std::endl;
            }
        }
    }
};

std::vector<std::string> read_lines(const std::string &filepath)
{
    std::vector<std::string> lines;
    std::ifstream infile(filepath);

    if (!infile)
    {
        std::cerr << "Error: Cannot open file " << filepath << std::endl;
        return lines;
    }

    std::string line;
    while (std::getline(infile, line))
    {
        if (!line.empty())
        {
            // remove any trailing \n\r
            line.erase(line.find_last_not_of("\n\r") + 1);
            lines.push_back(line);
        }
    }

    if (!lines.empty())
    {
        int first_length = lines[0].length();
        bool consistent_length = true;
        for (const auto &barcode : lines)
        {
            if (barcode.length() != first_length)
            {
                consistent_length = false;
                break;
            }
        }

        if (!consistent_length)
        {
            std::cerr << "Warning: Inconsistent barcode lengths detected. "
                      << "DecodeIndex2 and TableSearch require consistent lengths." << std::endl;
        }
        else
        {
            std::cout << "All barcodes have consistent length: " << first_length << std::endl;
        }
    }

    return lines;
}

// Example usage function
void testHugeClusterer()
{
    const int MEGA_CLUSTER_SIZE = 500 * 500;   // objects per mega-cluster
    const int CENTROIDS_PER_MEGA = 500;        // centroids per mega-cluster
    const int FINAL_CLUSTERS = 500;            // final clusters
    const int N_ITERATIONS = 5;                // Number of hierarchical iterations
    std::vector<int> itrs_to_save_on = {3, 5}; // Specify iterations to save on

    // Load your data (replace with actual data file)
    std::vector<std::string> data = read_lines(INDICES_FILE_PATH);

    if (data.empty())
    {
        std::cout << "No data loaded. Creating sample data..." << std::endl;
        return;
    }

    std::cout << "Loaded " << data.size() << " data points" << std::endl;

    // Initialize hierarchical clusterer
    HierarchicalClusterer clusterer(
        MEGA_CLUSTER_SIZE,
        CENTROIDS_PER_MEGA,
        FINAL_CLUSTERS,
        100, // max iterations for each clustering
        42   // random seed
    );

    // Measure total clustering time
    auto start_time = std::chrono::high_resolution_clock::now();

    // Fit the clusterer
    clusterer.fit(data, N_ITERATIONS, true, itrs_to_save_on);

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;
    std::cout << "Total clustering time: " << duration.count() << " seconds" << std::endl;

    std::cout << "\nHuge clustering completed successfully!" << std::endl;
}

int main()
{
    testHugeClusterer();
    return 0;
}