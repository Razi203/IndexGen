#pragma once
#include <vector>
#include <string>

namespace indexgen {
namespace clustering {

class IClustering {
public:
    virtual ~IClustering() = default;
    
    // Main clustering method: takes a list of strings and returns a list of clusters (list of strings)
    virtual std::vector<std::vector<std::string>> cluster(const std::vector<std::string>& data) = 0;
};

} // namespace clustering
} // namespace indexgen
