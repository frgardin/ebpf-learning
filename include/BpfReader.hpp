#pragma once
#include <bpf/bpf.h>
#include <string>
#include <unordered_map>
#include <stdexcept>

class BpfReader
{
private:
    int mapFd;
    std::string mapPath;

public:
    explicit BpfReader(const std::string &pinnedPath);
    std::unordered_map<int, long> readAll();
};
