#pragma once
#include <bpf/bpf.h>
#include <string>
#include <unordered_map>
#include <stdexcept>

class BpfSyscallFrequencyReader
{
private:
    int mapFd;
    std::string mapPath;

public:
    explicit BpfSyscallFrequencyReader();
    std::unordered_map<int, long> readAll();
    std::string getSyscallName(int key);
};
