#pragma once
#include <iostream>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "Logger.hpp"

struct data_t {
    int pid;
    int uid;
    char command[16];
    char message[12];
};

class RingBufReader {
private:
    std::string map_path;
    int map_fd;
    struct ring_buffer *rb;
    std::atomic<bool> running;
    std::thread read_thread;
    
    static int handle_ringbuf_event(void *ctx, void *data, size_t size);
    
public:
    using EventCallback = std::function<void(const data_t&)>;
    
    RingBufReader(const std::string &pinned_path = "/sys/fs/bpf/output");
    ~RingBufReader();
    
    bool open();
    void close();
    void start_reading(EventCallback callback);
    void stop_reading();
    bool is_running() const { return running; }
    
private:
    EventCallback user_callback;
    void read_loop();
};