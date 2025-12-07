#ifndef HDEQED_HPP
#define HDEQED_HPP

#include <string>
#include <vector>

// Public API
// ------------------------------------------------------------------
// 50x faster than CPL. Returns a consensus string that approximately
// minimizes the sum of edit distances to all cluster strings.
std::string HDEQEDFixMinSumFast(const std::vector<std::string> &cluster, int designLen);

// 15x faster than CPL. Returns a consensus string based on corrected
// clusters, with sum of edit distances slightly smaller than CPL.
std::string HDEQEDMinSumOfCorrectedClusterFast(const std::vector<std::string> &cluster, int designLen);

#endif // HDEQED_HPP
