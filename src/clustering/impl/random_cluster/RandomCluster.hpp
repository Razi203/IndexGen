#pragma once
#include <vector>
#include <string>

namespace indexgen {
namespace clustering {
namespace impl {

class RandomCluster {
private:
    int k;
public:
    RandomCluster(int k);
    std::vector<std::vector<std::string>> fit(const std::vector<std::string>& data);
};

} // namespace impl
} // namespace clustering
} // namespace indexgen
