extern "C" {
    #include "bpf/bpf.h"
}

#include <iostream>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    int map_id = 123;

    if (argc > 1) {
        map_id = std::stoi(argv[1]);
    }
     
    int map_fd = bpf_map_get_fd_by_id(map_id);
    if (map_fd < 0) {
        std::cerr << "Failed to get map FD. Make sure:" << std::endl;
        std::cerr << "1. The BPF program is loaded" << std::endl;
        std::cerr << "2. You're running as root" << std::endl;
        std::cerr << "3. The map ID is correct" << std::endl;
        return 1;
    }

    int key = 0;
    long value;
    
    if (bpf_map_lookup_elem(map_fd, &key, &value) == 0) {
        std::cout << "Current syscall count: " << value << std::endl;
    } else {
        std::cerr << "Failed to read from map" << std::endl;
    }

    close(map_fd);
    return 0;
}