#include "MetricProcessor.hpp"

std::unordered_map<std::string, double> MetricProcessor::process(const std::unordered_map<int, long>& raw) {
    std::unordered_map<std::string, double> result;
    for (auto& [k, v] : raw) {
        result["metric_" + std::to_string(k)] = static_cast<double>(v);
    }
    return result;
}
