#pragma once
#include <iostream>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "Logger.hpp"

// CPU event structure
struct cpu_event {
    __u32 pid;
    __u32 tgid;
    __u64 timestamp;
    char comm[16];
    __u64 runtime_ns;
    __u32 cpu_id;
};

// Memory event structure
struct memory_event {
    __u32 pid;
    __u32 tgid;
    __u64 timestamp;
    char comm[16];
    __u64 rss_kb;
    __u64 cache_kb;
    __u32 event_type;
};

// Event types
#define EVENT_CPU_USAGE       1
#define EVENT_MEMORY_ALLOC    2
#define EVENT_MEMORY_FREE     3
#define EVENT_MEMORY_REPORT   4

class RingBufReaderK8s {
private:
    std::string cpu_map_path;
    std::string memory_map_path;
    int cpu_map_fd;
    int memory_map_fd;
    struct ring_buffer *cpu_rb;
    struct ring_buffer *memory_rb;
    std::atomic<bool> running;
    std::thread read_thread;
    
    static int handle_cpu_event(void *ctx, void *data, size_t size);
    static int handle_memory_event(void *ctx, void *data, size_t size);
    
public:
    using CpuEventCallback = std::function<void(const cpu_event&)>;
    using MemoryEventCallback = std::function<void(const memory_event&)>;
    
    RingBufReaderK8s(const std::string &cpu_pinned_path = "/sys/fs/bpf/cpu_events",
                    const std::string &memory_pinned_path = "/sys/fs/bpf/memory_events");
    ~RingBufReaderK8s();
    
    bool open();
    void close();
    void start_reading(CpuEventCallback cpu_callback, MemoryEventCallback memory_callback);
    void stop_reading();
    bool is_running() const { return running; }
    
private:
    CpuEventCallback cpu_callback;
    MemoryEventCallback memory_callback;
    void read_loop();
};