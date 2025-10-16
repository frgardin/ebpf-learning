cmake_minimum_required(VERSION 3.16)
project(ebpf_observer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------
# libbpf
# -----------------------------
find_path(LIBBPF_INCLUDE_DIR bpf/libbpf.h)
find_library(LIBBPF_LIBRARY bpf)
find_library(BPF_LIBRARY bpf)

include_directories(${LIBBPF_INCLUDE_DIR} include)

# -----------------------------
# influxdb-cpp (header-only library)
# -----------------------------
include(FetchContent)
FetchContent_Declare(
    influxdb-cpp
    GIT_REPOSITORY https://github.com/orca-zhang/influxdb-cpp.git
)
FetchContent_MakeAvailable(influxdb-cpp)

# Create an interface library target for the header-only library
add_library(influxdb-cpp INTERFACE)
target_include_directories(influxdb-cpp INTERFACE ${influxdb-cpp_SOURCE_DIR})

# -----------------------------
# Main executable
# -----------------------------
add_executable(ebpf-observer
    src/main.cpp
    src/BpfReader.cpp
    src/MetricProcessor.cpp
    src/InfluxClient.cpp
    src/Logger.cpp
)

# -----------------------------
# Link libraries
# -----------------------------
target_link_libraries(ebpf-observer
    PRIVATE
        ${LIBBPF_LIBRARY}
        ${BPF_LIBRARY}
        elf
        z
        m
        influxdb-cpp  # Now links to the interface target
)

# Include directories for your target
target_include_directories(ebpf-observer 
    PRIVATE 
        ${LIBBPF_INCLUDE_DIR}
        include
)