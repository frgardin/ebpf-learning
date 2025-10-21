#include "RingBufReaderK8s.hpp"
#include <cstring>
#include <stdexcept>
#include <unistd.h>

RingBufReaderK8s::RingBufReaderK8s(const std::string &pinned_path) 
    : map_path(pinned_path), map_fd(-1), rb(nullptr), running(false) {
}

RingBufReaderK8s::~RingBufReaderK8s() {
    stop_reading();
    close();
}

bool RingBufReaderK8s::open() {
    map_fd = bpf_obj_get(map_path.c_str());
    if (map_fd < 0) {
        Logger::error("Failed to open ring buffer map: " + map_path);
        return false;
    }
    
    rb = ring_buffer__new(map_fd, handle_ringbuf_event, this, nullptr);
    if (!rb) {
        Logger::error("Failed to create ring buffer");
        ::close(map_fd);
        map_fd = -1;
        return false;
    }
    
    Logger::info("Ring buffer opened successfully: " + map_path);
    return true;
}

void RingBufReaderK8s::close() {
    if (rb) {
        ring_buffer__free(rb);
        rb = nullptr;
    }
    if (map_fd >= 0) {
        ::close(map_fd);
        map_fd = -1;
    }
}

int RingBufReaderK8s::handle_ringbuf_event(void *ctx, void *data, size_t size) {
    RingBufReaderK8s *reader = static_cast<RingBufReaderK8s*>(ctx);
    
    if (size != sizeof(k8s_perf_event)) {
        Logger::warn("Unexpected data size: " + std::to_string(size) + " expected: " + std::to_string(sizeof(k8s_perf_event)));
        return 0;
    }
    
    k8s_perf_event *event_data = static_cast<k8s_perf_event*>(data);
    
    if (reader->user_callback) {
        reader->user_callback(*event_data);
    }
    
    return 0;
}

void RingBufReaderK8s::start_reading(EventCallback callback) {
    if (!rb) {
        throw std::runtime_error("Ring buffer not opened");
    }
    
    if (running) {
        Logger::warn("Ring buffer reader already running");
        return;
    }
    
    user_callback = callback;
    running = true;
    read_thread = std::thread(&RingBufReaderK8s::read_loop, this);
    
    Logger::info("Ring buffer reader started");
}

void RingBufReaderK8s::stop_reading() {
    if (running) {
        running = false;
        if (read_thread.joinable()) {
            read_thread.join();
        }
        Logger::info("Ring buffer reader stopped");
    }
}

void RingBufReaderK8s::read_loop() {
    const int timeout_ms = 100;
    
    while (running) {
        int err = ring_buffer__poll(rb, timeout_ms);
        
        if (err < 0) {
            if (err != -EINTR) {
                Logger::error("Error polling ring buffer: " + std::to_string(err));
                break;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}