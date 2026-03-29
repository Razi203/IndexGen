#pragma once
#include <vector>
#include <string>
#include <memory>
#include "clustering/ClusteringInterface.hpp"
#include "impl/hierarchical_kmeans/HierarchicalAdjKMeans.hpp"

namespace indexgen {
namespace clustering {

class KMeansAdapter : public IClustering {
private:
    int k;
    std::string method;
    bool isBinary;
public:
    KMeansAdapter(int k, const std::string& method = "hierarchical_kmeans", bool isBinary = false);
    std::vector<std::vector<std::string>> cluster(const std::vector<std::string>& data) override;
};

} // namespace clustering
} // namespace indexgen
