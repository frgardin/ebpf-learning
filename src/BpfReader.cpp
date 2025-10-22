#include "BpfReader.hpp"
#include <iostream>

BpfReader::BpfReader(const std::string &pinnedPath) : mapPath(pinnedPath)
{
    mapFd = bpf_obj_get(mapPath.c_str());
    if (mapFd < 0)
    {
        throw std::runtime_error("Erro ao abrir mapa BPF: " + mapPath);
    }
}

std::unordered_map<int, long> BpfReader::readAll()
{
    std::unordered_map<int, long> data;
    int key = 0, nextKey = 0;
    long value = 0;

    if (bpf_map_lookup_elem(mapFd, &key, &value) == 0)
    {
        data[key] = value;
    }

    while (bpf_map_get_next_key(mapFd, &key, &nextKey) == 0)
    {
        if (bpf_map_lookup_elem(mapFd, &nextKey, &value) == 0)
        {
            data[nextKey] = value;
        }
        key = nextKey;
    }

    return data;
}
