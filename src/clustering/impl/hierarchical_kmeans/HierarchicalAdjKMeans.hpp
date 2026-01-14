#pragma once
#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "CPL/DNAV8_amir.hpp"
#include "pipeline_utils.hpp"
#include "HDEQED.hpp"

namespace indexgen {
namespace clustering {
namespace impl {

using namespace std;
// Global variable in original script, making it internal or static
static string CENTROID_TYPE = "HDEQEDMinSumOfCorrectedClusterFast"; 
static int INDEX_LEN = 18;                                                
static const string INDICES_FILE_PATH = "unused"; // Removing original hardcoded path usage


inline std::vector<std::string> read_lines(const std::string &filepath)
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
        size_t first_length = lines[0].length();
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

class StringKMeans
{
  private:
    int k;
    int max_iterations;
    std::vector<std::string> data;
    std::vector<int> assignments;
    std::vector<std::string> centroids;
    std::mt19937 rng;

    // Timing variables
    mutable std::vector<double> centroid_calculation_times;

  public:
    StringKMeans(int k, int max_iter = 100, int seed = 42) : k(k), max_iterations(max_iter), rng(seed)
    {
    }

    std::string calculateCentroid_min_edit_distance(const std::vector<std::string> &cluster_strings)
    {
        if (cluster_strings.empty())
        {
            return "";
        }

        if (cluster_strings.size() == 1)
        {
            return cluster_strings[0];
        }

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
        auto start_time = std::chrono::high_resolution_clock::now();

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

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end_time - start_time;
        centroid_calculation_times.push_back(duration.count());

        return result;
    }

    void initializeCentroids()
    {
        centroids.clear();
        centroids.reserve(k);

        std::vector<int> indices(data.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), rng);

        for (size_t i = 0; i < static_cast<size_t>(k) && i < data.size(); ++i)
        {
            centroids.push_back(data[indices[i]]);
        }

        while (centroids.size() < static_cast<size_t>(k))
        {
            centroids.push_back(data[rng() % data.size()]);
        }
    }

    bool assignToClusters()
    {
        std::vector<int> new_assignments(data.size());
        bool changed = false;

        for (size_t i = 0; i < data.size(); ++i)
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

        for (size_t i = 0; i < data.size(); ++i)
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
        centroid_calculation_times.clear(); // Reset timing data

        if (data.empty())
        {
            return assignments;
        }

        initializeCentroids();

        for (int iter = 0; iter < max_iterations; ++iter)
        {
            cout << "Starting iteration " << iter + 1 << "...\n";
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

    std::vector<std::string> getCentroids() const
    {
        return centroids;
    }

    double getAverageCentroidCalculationTime() const
    {
        if (centroid_calculation_times.empty())
        {
            return 0.0;
        }

        double sum = std::accumulate(centroid_calculation_times.begin(), centroid_calculation_times.end(), 0.0);
        return sum / centroid_calculation_times.size();
    }

    size_t getCentroidCalculationCount() const
    {
        return centroid_calculation_times.size();
    }

    std::vector<double> getCentroidCalculationTimes() const
    {
        return centroid_calculation_times;
    }
};

// Simple structure to represent a cluster
struct SimpleCluster
{
    vector<int> data_indices; // Indices into original data
    string centroid;          // Centroid of this cluster

    SimpleCluster() = default;
    SimpleCluster(const vector<int> &indices, const string &cent) : data_indices(indices), centroid(cent)
    {
    }
};

class GeneralizedHierarchicalStringKMeans
{
  private:
    vector<int> hierarchy; // h = [k1, k2, k3, ...] for h levels
    int max_iterations;
    mt19937 rng;
    vector<string> data;
    vector<SimpleCluster> final_clusters;

    // Timing variables
    vector<double> all_centroid_calculation_times;
    size_t total_centroid_calculations;

  public:
    GeneralizedHierarchicalStringKMeans(const vector<int> &h, int max_iter = 100, int seed = 42)
        : hierarchy(h), max_iterations(max_iter), rng(seed), total_centroid_calculations(0)
    {
        if (hierarchy.empty())
        {
            throw invalid_argument("Hierarchy cannot be empty");
        }
    }

    void fit(const vector<string> &input_data)
    {
        data = input_data;
        all_centroid_calculation_times.clear();
        total_centroid_calculations = 0;


        if (data.empty())
        {
            return;
        }

        INDEX_LEN = data[0].length();

        cout << "Starting hierarchical clustering with hierarchy: [";
        for (size_t i = 0; i < hierarchy.size(); ++i)
        {
            cout << hierarchy[i];
            if (i < hierarchy.size() - 1)
                cout << ", ";
        }
        cout << "]" << endl;

        // Initialize with all data as one cluster
        vector<SimpleCluster> current_clusters;
        vector<int> all_indices(data.size());
        iota(all_indices.begin(), all_indices.end(), 0);
        current_clusters.emplace_back(all_indices, "");

        // Process each level in the hierarchy
        for (size_t level = 0; level < hierarchy.size(); ++level)
        {
            int k = hierarchy[level];
            cout << "Processing level " << level << " with k=" << k << " on " << current_clusters.size() << " clusters"
                 << endl;

            vector<SimpleCluster> next_clusters;

            // Process each cluster from current level
            for (const auto &cluster : current_clusters)
            {
                if (cluster.data_indices.empty())
                {
                    continue;
                }

                // Extract data for this cluster
                vector<string> cluster_data;
                for (int idx : cluster.data_indices)
                {
                    cluster_data.push_back(data[idx]);
                }

                // Skip if not enough data points
                if (cluster_data.size() <= 1)
                {
                    // Keep as single cluster
                    next_clusters.emplace_back(cluster.data_indices, cluster_data[0]);
                    continue;
                }

                // Run k-means on this cluster
                int effective_k = min(k, (int)cluster_data.size());
                StringKMeans kmeans(effective_k, max_iterations, rng());
                vector<int> assignments = kmeans.fit(cluster_data);
                vector<string> centroids = kmeans.getCentroids();

                // Collect timing data from this k-means run
                vector<double> kmeans_times = kmeans.getCentroidCalculationTimes();
                all_centroid_calculation_times.insert(all_centroid_calculation_times.end(), kmeans_times.begin(),
                                                      kmeans_times.end());
                total_centroid_calculations += kmeans.getCentroidCalculationCount();

                // Create new clusters from k-means results
                vector<SimpleCluster> subclusters(centroids.size());
                for (size_t i = 0; i < centroids.size(); ++i)
                {
                    subclusters[i].centroid = centroids[i];
                }

                // Assign data points to subclusters
                for (size_t i = 0; i < assignments.size(); ++i)
                {
                    int subcluster_id = assignments[i];
                    int original_idx = cluster.data_indices[i];
                    subclusters[subcluster_id].data_indices.push_back(original_idx);
                }

                // Add subclusters to next level
                for (const auto &subcluster : subclusters)
                {
                    if (!subcluster.data_indices.empty())
                    {
                        next_clusters.push_back(subcluster);
                    }
                }
            }

            // Update current clusters for next iteration
            current_clusters = move(next_clusters);
        }

        // Store final result
        final_clusters = move(current_clusters);

        cout << "Hierarchical clustering completed with " << final_clusters.size() << " final clusters!" << endl;
    }

    double getAverageCentroidCalculationTime() const
    {
        if (all_centroid_calculation_times.empty())
        {
            return 0.0;
        }

        double sum = std::accumulate(all_centroid_calculation_times.begin(), all_centroid_calculation_times.end(), 0.0);
        return sum / all_centroid_calculation_times.size();
    }

    size_t getTotalCentroidCalculationCount() const
    {
        return total_centroid_calculations;
    }

    std::vector<std::vector<std::string>> getTraversableClusters() const {
        std::vector<std::vector<std::string>> result;
        for (const auto& cluster : final_clusters) {
            std::vector<std::string> current_cluster_data;
            for (int idx : cluster.data_indices) {
                current_cluster_data.push_back(data[idx]);
            }
            if(!current_cluster_data.empty())
                result.push_back(current_cluster_data);
        }
        return result;
    }


    void save_clusters_to_file(const string &filename) const
    {
        ofstream outfile(filename);
        if (!outfile)
        {
            cerr << "Error: Cannot open file " << filename << " for writing." << endl;
            return;
        }

        cout << "Saving " << final_clusters.size() << " clusters to file." << endl;

        for (const auto &cluster : final_clusters)
        {
            // Write centroid
            outfile << cluster.centroid << "\n";
            outfile << "*************\n";

            // Write all data points in this cluster
            for (int idx : cluster.data_indices)
            {
                outfile << data[idx] << "\n";
            }
            outfile << "\n";
        }
    }

    void evaluateClustering() const
    {
        std::cout << std::fixed << std::setprecision(4);
        std::cout << "=== CLUSTERING EVALUATION METRICS ===\n\n";

        // Basic statistics
        int n_samples = data.size();
        int n_clusters = final_clusters.size();
        std::vector<int> cluster_sizes;
        for (const auto &cluster : final_clusters)
        {
            cluster_sizes.push_back(cluster.data_indices.size());
        }

        double avg_cluster_size = std::accumulate(cluster_sizes.begin(), cluster_sizes.end(), 0.0) / n_clusters;

        // Calculate cluster size standard deviation
        double cluster_size_variance = 0.0;
        for (int size : cluster_sizes)
        {
            cluster_size_variance += (size - avg_cluster_size) * (size - avg_cluster_size);
        }
        double cluster_size_std = std::sqrt(cluster_size_variance / n_clusters);

        int min_cluster_size = *std::min_element(cluster_sizes.begin(), cluster_sizes.end());
        int max_cluster_size = *std::max_element(cluster_sizes.begin(), cluster_sizes.end());

        std::cout << "Dataset size: " << n_samples << std::endl;
        std::cout << "Number of clusters: " << n_clusters << std::endl;
        std::cout << "Average cluster size: " << avg_cluster_size << std::endl;
        std::cout << "Cluster size std: " << cluster_size_std << std::endl;
        std::cout << "Min/Max cluster size: " << min_cluster_size << "/" << max_cluster_size << "\n\n";

        // Core metrics
        std::cout << "=== CORE METRICS ===" << std::endl;
        // calculate wcss, but report max,min,avg distance to centroid
        double min_distance = std::numeric_limits<double>::max();
        double max_distance = 0.0;
        double total_distance = 0.0;
        int count = 0;

        for (const auto &cluster : final_clusters)
        {
            for (int data_idx : cluster.data_indices)
            {
                double distance = FastEditDistance(data[data_idx], cluster.centroid);
                min_distance = std::min(min_distance, distance);
                max_distance = std::max(max_distance, distance);
                total_distance += distance;
                count++;
            }
        }

        if (count > 0)
        {
            double avg_distance = total_distance / count;
            std::cout << "Min/Max/Avg distance to centroid: " << min_distance << "/" << max_distance << "/"
                      << avg_distance << std::endl;
        }

        // calculate and print min,max,avg distance between centroids
        double min_centroid_distance = std::numeric_limits<double>::max();
        double max_centroid_distance = 0.0;
        double total_centroid_distance = 0.0;
        int centroid_distance_count = 0;

        std::vector<double> centroid_distance_vector;

        for (int i = 0; i < n_clusters; ++i)
        {
            for (int j = i + 1; j < n_clusters; ++j)
            {
                double dist = FastEditDistance(final_clusters[i].centroid, final_clusters[j].centroid);
                min_centroid_distance = std::min(min_centroid_distance, dist);
                max_centroid_distance = std::max(max_centroid_distance, dist);
                total_centroid_distance += dist;
                centroid_distance_count++;
                centroid_distance_vector.push_back(dist);
            }
        }

        double avg_centroid_distance =
            (centroid_distance_count > 0) ? (total_centroid_distance / centroid_distance_count) : 0.0;

        std::cout << "Centroid distances (min/max/avg): " << min_centroid_distance << "/" << max_centroid_distance
                  << "/" << avg_centroid_distance << "\n\n";

        // Print histogram- make bin for each distance value
        int num_bins = max_centroid_distance - min_centroid_distance + 1;
        std::vector<int> histogram(num_bins, 0);

        for (double dist : centroid_distance_vector)
        {
            int bin_index = static_cast<int>((dist - min_centroid_distance));
            if (bin_index >= 0 && bin_index < num_bins)
            {
                histogram[bin_index]++;
            }
        }

        std::cout << "Histogram of centroid distances:\n";
        for (int i = 0; i < num_bins; ++i)
        {
            std::cout << (min_centroid_distance + i) << ": " << histogram[i] << "\n";
        }

        int x = 10; // number of closest centroid pairs to consider

        // take top x couples of closest centroids, and for each couple find the closest indexes from the clusters (1
        // from each cluster), report the min distance between them
        std::vector<std::pair<int, int>> closest_centroid_pairs;
        for (int i = 0; i < n_clusters; ++i)
        {
            for (int j = i + 1; j < n_clusters; ++j)
            {
                closest_centroid_pairs.emplace_back(i, j);
            }
        }
        std::sort(closest_centroid_pairs.begin(), closest_centroid_pairs.end(),
                  [&](const std::pair<int, int> &a, const std::pair<int, int> &b)
                  {
                      return FastEditDistance(final_clusters[a.first].centroid, final_clusters[a.second].centroid) <
                             FastEditDistance(final_clusters[b.first].centroid, final_clusters[b.second].centroid);
                  });
        closest_centroid_pairs.resize(std::min(x, static_cast<int>(closest_centroid_pairs.size())));

        double min_distance_between_samples = std::numeric_limits<double>::max();
        for (const auto &pair : closest_centroid_pairs)
        {
            int c1 = pair.first;
            int c2 = pair.second;

            for (int data_idx1 : final_clusters[c1].data_indices)
            {
                for (int data_idx2 : final_clusters[c2].data_indices)
                {
                    double distance;
                    if (min_distance_between_samples != std::numeric_limits<double>::max())
                    {
                        distance = FastEditDistanceWithThresholdBanded(data[data_idx1], data[data_idx2],
                                                                       min_distance_between_samples);
                    }
                    else
                    {
                        distance = FastEditDistance(data[data_idx1], data[data_idx2]);
                    }
                    if (distance < min_distance_between_samples)
                    {
                        min_distance_between_samples = distance;
                    }
                }
            }
        }
        std::cout << "Min distance between closest samples from closest centroids: " << min_distance_between_samples
                  << "\n\n";
    }
};

inline void testGeneralizedHierarchicalStringKMeans(const std::string &input_file, vector<int> h = {500})
{
    vector<string> strings = read_lines(input_file);

    if (strings.empty())
    {
        cerr << "Error: No strings loaded from " << input_file << endl;
        return;
    }

    // Set global INDEX_LEN based on input
    INDEX_LEN = strings[0].length();
    // cout << "Dynamically set INDEX_LEN to " << INDEX_LEN << endl;

    GeneralizedHierarchicalStringKMeans hkmeans(h);

    auto start_time = chrono::high_resolution_clock::now();
    hkmeans.fit(strings);
    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end_time - start_time;

    cout << "Hierarchical clustering time: " << duration.count() << " seconds" << endl;

    // hkmeans.evaluateClustering();
    string h_str = "";
    for (size_t i = 0; i < h.size(); i++)
    {
        h_str += to_string(h[i]);
        if (i != h.size() - 1)
        {
            h_str += ",";
        }
    }
    string filename = "exps_results/indices_clusters_" + h_str + "_K_" + CENTROID_TYPE + ".txt";
    hkmeans.save_clusters_to_file(filename);

    // Print centroid calculation timing statistics
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Centroid calculation statistics:" << std::endl;
    std::cout << "  Total centroid calculations: " << hkmeans.getTotalCentroidCalculationCount() << std::endl;
    std::cout << "  Average time per centroid calculation: " << hkmeans.getAverageCentroidCalculationTime()
              << " seconds" << std::endl;

    cout << "Done generalized hierarchical clustering!" << endl;
} // closes function testGeneralized...

} // namespace impl
} // namespace clustering
} // namespace indexgen

