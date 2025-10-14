#pragma once
#include <unordered_map>
#include <string>

class MetricProcessor {
public:
    static std::unordered_map<std::string, double> process(const std::unordered_map<int, long>& raw);
};
