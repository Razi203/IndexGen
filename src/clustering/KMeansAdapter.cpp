#include "KMeansAdapter.hpp"

namespace indexgen {
namespace clustering {

KMeansAdapter::KMeansAdapter(int k) : k(k) {}

std::vector<std::vector<std::string>> KMeansAdapter::cluster(const std::vector<std::string>& data) {
    if (data.empty()) return {};

    // Use a default hierarchy for now, or make it configurable
    // 500 is the default from the original script
    std::vector<int> h = {k}; 
    
    // We instantiate the implementation class
    // Note: The implementation class prints to stdout, we might want to suppress that later
    impl::GeneralizedHierarchicalStringKMeans hkmeans(h);
    
    // fit() runs the clustering
    // Suppress output by redirecting cout buffer
    std::streambuf* oldCoutStreamBuf = std::cout.rdbuf();
    std::ostringstream dummyStream;
    std::cout.rdbuf(dummyStream.rdbuf());
    
    try {
        hkmeans.fit(data);
    }
    catch (...) {
        // Restore stdout if exception occurs
        std::cout.rdbuf(oldCoutStreamBuf);
        throw;
    }
    
    // Restore stdout
    std::cout.rdbuf(oldCoutStreamBuf);
    
    // Return result in standard format
    return hkmeans.getTraversableClusters();
}

} // namespace clustering
} // namespace indexgen
