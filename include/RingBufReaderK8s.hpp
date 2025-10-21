#pragma once
#include <iostream>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "Logger.hpp"

struct k8s_perf_event {
    int pid;
    int uid;
    unsigned long long timestamp;
    int event_type;
    unsigned long long duration_ns;
    unsigned long long bytes;
    int syscall_id;
    char command[16];
    char message[32];
};

// Event types
#define EVENT_SYSCALL_ENTER   1
#define EVENT_SYSCALL_EXIT    2
#define EVENT_SCHED_IN        3
#define EVENT_SCHED_OUT       4
#define EVENT_NET_RX          5
#define EVENT_NET_TX          6
#define EVENT_FILE_READ       7
#define EVENT_FILE_WRITE      8

class RingBufReaderK8s {
private:
    std::string map_path;
    int map_fd;
    struct ring_buffer *rb;
    std::atomic<bool> running;
    std::thread read_thread;
    
    static int handle_ringbuf_event(void *ctx, void *data, size_t size);
    
public:
    using EventCallback = std::function<void(const k8s_perf_event&)>;
    
    RingBufReaderK8s(const std::string &pinned_path = "/sys/fs/bpf/output");
    ~RingBufReaderK8s();
    
    bool open();
    void close();
    void start_reading(EventCallback callback);
    void stop_reading();
    bool is_running() const { return running; }
    
private:
    EventCallback user_callback;
    void read_loop();
};