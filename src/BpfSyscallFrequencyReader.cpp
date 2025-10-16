#include "BpfSyscallFrequencyReader.hpp"
#include <iostream>
#include "Logger.hpp"

BpfSyscallFrequencyReader::BpfSyscallFrequencyReader()
{
    mapPath="/sys/fs/bpf/syscall_counts";
    mapFd = bpf_obj_get(mapPath.c_str());
    if (mapFd < 0)
    {
        throw std::runtime_error("Erro ao abrir mapa BPF: " + mapPath);
    }
}
std::unordered_map<int, long> BpfSyscallFrequencyReader::readAll()
{
    std::unordered_map<int, long> data;
    
    // For array maps, iterate sequentially through all possible keys
    for (int key = 0; key < 8; key++) {
        long value = 0;
        if (bpf_map_lookup_elem(mapFd, &key, &value) == 0) {
            data[key] = value;
            Logger::debug(std::format("Found key {} with value {}", key, value));
        }
    }
    
    return data;
}

// Helper function to get syscall name from key
std::string BpfSyscallFrequencyReader::getSyscallName(int key) {
    switch (key) {
        case 0: return "read";
        case 1: return "write"; 
        case 2: return "openat";
        case 3: return "close";
        case 4: return "mmap";
        case 5: return "brk";
        case 6: return "getpid";
        case 7: return "clock_gettime";
        default: return "unknown";
    }
}