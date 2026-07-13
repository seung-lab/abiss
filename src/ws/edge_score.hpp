#pragma once

#include <algorithm>
#include <cctype>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

enum class EdgeScoreMode {
    MAX,
    MEAN,
    PERCENTILE,
};

struct EdgeScoreConfig {
    EdgeScoreMode mode = EdgeScoreMode::MAX;
    float percentile = 0.0f;  // [0,100], only used when mode == PERCENTILE
};

// Parse CLI string to config.
// Accepted formats: "max", "mean", "pNN" (e.g. "p75", "p90.5")
inline EdgeScoreConfig parse_edge_score_config(const std::string& s) {
    EdgeScoreConfig cfg;
    if (s == "max") {
        cfg.mode = EdgeScoreMode::MAX;
    } else if (s == "mean") {
        cfg.mode = EdgeScoreMode::MEAN;
    } else if (s.size() >= 2 && s[0] == 'p' && (std::isdigit(s[1]) || s[1] == '.')) {
        cfg.mode = EdgeScoreMode::PERCENTILE;
        cfg.percentile = std::stof(s.substr(1));
        if (cfg.percentile < 0.0f || cfg.percentile > 100.0f) {
            throw std::invalid_argument(
                "Percentile must be in [0, 100], got " + std::to_string(cfg.percentile));
        }
    } else {
        throw std::invalid_argument(
            "Unknown merge function: '" + s + "'. Use 'max', 'mean', or 'pNN' (e.g. 'p75').");
    }
    return cfg;
}

// Compute a single score from a vector of affinities for one edge pair.
// The vector may be reordered (nth_element is not stable).
template <typename F>
inline F compute_edge_score(std::vector<F>& affinities, const EdgeScoreConfig& cfg) {
    if (affinities.empty()) return F(0);

    switch (cfg.mode) {
        case EdgeScoreMode::MAX:
            return *std::max_element(affinities.begin(), affinities.end());

        case EdgeScoreMode::MEAN: {
            F sum = std::accumulate(affinities.begin(), affinities.end(), F(0));
            return sum / static_cast<F>(affinities.size());
        }

        case EdgeScoreMode::PERCENTILE: {
            size_t n = affinities.size();
            size_t k = static_cast<size_t>(
                cfg.percentile / 100.0f * static_cast<float>(n - 1) + 0.5f);
            if (k >= n) k = n - 1;
            std::nth_element(affinities.begin(), affinities.begin() + k, affinities.end());
            return affinities[k];
        }
    }
    return F(0);
}
