#include "KMeansAdapter.hpp"
#include "impl/random_cluster/RandomCluster.hpp"
#include <cstdlib>

namespace indexgen
{
namespace clustering
{

KMeansAdapter::KMeansAdapter(int k, const std::string& method) : k(k), method(method)
{
}

std::vector<std::vector<std::string>> KMeansAdapter::cluster(const std::vector<std::string> &data)
{
    if (data.empty())
        return {};

    if (method == "random_cluster")
    {
        impl::RandomCluster random_cluster(k);
        return random_cluster.fit(data);
    }

    // Default hierarchy
    std::vector<int> h = {k};

    // We instantiate the implementation class
    // Note: The implementation class prints to stdout, we might want to suppress that later
    impl::GeneralizedHierarchicalStringKMeans hkmeans(h);

    // fit() runs the clustering
    // Suppress output by redirecting cout buffer
    std::streambuf *oldCoutStreamBuf = std::cout.rdbuf();
    std::ostringstream dummyStream;
    std::cout.rdbuf(dummyStream.rdbuf());

    try
    {
        hkmeans.fit(data);
    }
    catch (...)
    {
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
