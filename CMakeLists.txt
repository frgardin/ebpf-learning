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
# influxdb-cxx (via FetchContent)
# -----------------------------
include(FetchContent)
FetchContent_Declare(
  influxdb-cxx
  GIT_REPOSITORY https://github.com/offa/influxdb-cxx.git
  GIT_TAG v0.6.3
)
FetchContent_MakeAvailable(influxdb-cxx)

# -----------------------------
# Executável principal
# -----------------------------
add_executable(ebpf-observer
    src/main.cpp
    src/BpfReader.cpp
    src/MetricProcessor.cpp
    src/InfluxClient.cpp
)

# -----------------------------
# Linkagem
# -----------------------------
target_link_libraries(ebpf-observer
    PRIVATE
        ${LIBBPF_LIBRARY}
        ${BPF_LIBRARY}
        elf
        z
        m
        influxdb-cxx  # Use the actual target name, not the imported target
)