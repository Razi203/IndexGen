#pragma once
#include <vector>
#include <string>

namespace indexgen {
namespace clustering {
namespace impl {

class RandomCluster {
private:
    int k;
    bool isBinary;

public:
    RandomCluster(int k, bool isBinary = false);
    std::vector<std::vector<std::string>> fit(const std::vector<std::string>& data);
};

} // namespace impl
} // namespace clustering
} // namespace indexgen
