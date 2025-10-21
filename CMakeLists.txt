# =============================================================================
# eBPF Observer Project Configuration
# =============================================================================

# Minimum required CMake version
cmake_minimum_required(VERSION 3.16)

# Project definition with C++ as the primary language
project(ebpf_observer LANGUAGES CXX)

# C++ standard configuration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# Dependency: libbpf (BPF library for userspace interaction)
# =============================================================================

# Find libbpf header files
find_path(LIBBPF_INCLUDE_DIR bpf/libbpf.h)

# Find libbpf library
find_library(LIBBPF_LIBRARY bpf)

# Some distributions may have libbpf under a different name
find_library(BPF_LIBRARY bpf)

# Include libbpf headers in the build
include_directories(${LIBBPF_INCLUDE_DIR} include)

# =============================================================================
# Dependency: libcurl (HTTP client for InfluxDB communication)
# =============================================================================

# Find libcurl header files
find_path(CURL_INCLUDE_DIR curl/curl.h)

# Find libcurl library
find_library(CURL_LIBRARY curl)

# Include libcurl headers in the build
include_directories(${CURL_INCLUDE_DIR} include)

# =============================================================================
# Dependency: influxdb-cpp (Header-only C++ client for InfluxDB)
# =============================================================================

# Use CMake's FetchContent to download the library at configure time
include(FetchContent)
FetchContent_Declare(
    influxdb-cpp
    GIT_REPOSITORY https://github.com/orca-zhang/influxdb-cpp.git
    # Note: No GIT_TAG specified, uses latest master
)

# Download and make the library available
FetchContent_MakeAvailable(influxdb-cpp)

# Create an interface library target for the header-only influxdb-cpp
add_library(influxdb-cpp INTERFACE)

# Add the source directory to the include path for this target
target_include_directories(influxdb-cpp INTERFACE ${influxdb-cpp_SOURCE_DIR})

# =============================================================================
# Executable 1: eBPF Syscall Metrics Monitor
# 
# This executable monitors system call frequencies using eBPF and exports
# metrics to InfluxDB for analysis and visualization.
# =============================================================================

add_executable(ebpf-syscall-metrics
    # Main application entry point for syscall monitoring
    src/main_syscall_metrics.cpp
    
    # Core BPF functionality for reading map data
    src/BpfReader.cpp
    
    # Specialized reader for syscall frequency data
    src/BpfSyscallFrequencyReader.cpp
    
    # InfluxDB client for metrics export
    src/InfluxClient.cpp
    
    # Logging utilities
    src/Logger.cpp
)

# Link required libraries for the syscall metrics executable
target_link_libraries(ebpf-syscall-metrics
    PRIVATE
        # BPF libraries for eBPF interaction
        ${LIBBPF_LIBRARY}
        ${BPF_LIBRARY}
        
        # HTTP client for InfluxDB communication
        ${CURL_LIBRARY}
        
        # System libraries required by libbpf
        elf      # ELF file parsing
        z        # Compression library
        m        # Math library
        
        # Header-only InfluxDB client
        influxdb-cpp
)

# Specify include directories for this target
target_include_directories(ebpf-syscall-metrics
    PRIVATE 
        ${LIBBPF_INCLUDE_DIR}    # libbpf headers
        ${CURL_INCLUDE_DIR}      # libcurl headers
        include                  # Project-specific headers
)

# =============================================================================
# Executable 2: eBPF Hello Ring Buffer Demo
# 
# This executable demonstrates ring buffer usage for real-time event
# capture from eBPF programs, showing process information for system calls.
# =============================================================================

add_executable(ebpf-hello-ring-buffer
    # Main application entry point for ring buffer demo
    src/main_hello_ring_buffer.cpp
    
    # Ring buffer reader implementation
    src/RingBufReader.cpp
    
    # InfluxDB client (optional for this demo)
    src/InfluxClient.cpp
    
    # Logging utilities
    src/Logger.cpp
)

# Link required libraries for the ring buffer demo executable
target_link_libraries(ebpf-hello-ring-buffer
    PRIVATE
        # BPF libraries for eBPF interaction
        ${LIBBPF_LIBRARY}
        ${BPF_LIBRARY}
        
        # HTTP client for optional InfluxDB integration
        ${CURL_LIBRARY}
        
        # System libraries required by libbpf
        elf      # ELF file parsing
        z        # Compression library
        m        # Math library
        
        # Header-only InfluxDB client
        influxdb-cpp
)

# Specify include directories for this target
target_include_directories(ebpf-hello-ring-buffer
    PRIVATE 
        ${LIBBPF_INCLUDE_DIR}    # libbpf headers
        ${CURL_INCLUDE_DIR}      # libcurl headers
        include                  # Project-specific headers
)

# =============================================================================
# Build Configuration Notes:
# 
# - Both executables share common dependencies but serve different purposes:
#   1. ebpf-syscall-metrics: Production monitoring with metrics export
#   2. ebpf-hello-ring-buffer: Demo/development for ring buffer functionality
# 
# - The influxdb-cpp library is header-only, so we use an INTERFACE target
# - libbpf and libcurl are dynamically linked system libraries
# - Build with: `cmake -B build && cmake --build build`
# - Run with appropriate privileges: `sudo ./ebpf-syscall-metrics`
# =============================================================================