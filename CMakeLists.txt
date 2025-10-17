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
# curl
# -----------------------------
find_path(CURL_INCLUDE_DIR curl/curl.h)
find_library(CURL_LIBRARY curl)

include_directories(${CURL_INCLUDE_DIR} include)

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
add_executable(ebpf-syscall-metrics
    src/main_syscall_metrics.cpp
    src/BpfReader.cpp
    src/BpfSyscallFrequencyReader.cpp
    src/MetricProcessor.cpp
    src/InfluxClient.cpp
    src/Logger.cpp
)

# -----------------------------
# Link libraries
# -----------------------------
target_link_libraries(ebpf-syscall-metrics
    PRIVATE
        ${LIBBPF_LIBRARY}
        ${BPF_LIBRARY}
        ${CURL_LIBRARY}
        elf
        z
        m
        influxdb-cpp  # Now links to the interface target
)

# Include directories for your target
target_include_directories(ebpf-syscall-metrics 
    PRIVATE 
        ${LIBBPF_INCLUDE_DIR}
        ${CURL_INCLUDE_DIR}
        include
)


# -----------------------------
# Main hello ring buffer executable
# -----------------------------
add_executable(ebpf-hello-ring-buffer
    src/main_hello_ring_buffer.cpp
    src/RingBufReader.cpp
    src/InfluxClient.cpp
    src/Logger.cpp
)

# -----------------------------
# Link libraries
# -----------------------------
target_link_libraries(ebpf-hello-ring-buffer
    PRIVATE
        ${LIBBPF_LIBRARY}
        ${BPF_LIBRARY}
        ${CURL_LIBRARY}
        elf
        z
        m
        influxdb-cpp  # Now links to the interface target
)

# Include directories for your target
target_include_directories(ebpf-hello-ring-buffer 
    PRIVATE 
        ${LIBBPF_INCLUDE_DIR}
        ${CURL_INCLUDE_DIR}
        include
)