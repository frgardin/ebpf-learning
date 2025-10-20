#include "RingBufReader.hpp"
#include <cstring>
#include <stdexcept>
#include <unistd.h>

RingBufReader::RingBufReader(const std::string &pinned_path) 
    : map_path(pinned_path), map_fd(-1), rb(nullptr), running(false) {
}

RingBufReader::~RingBufReader() {
    stop_reading();
    close();
}

bool RingBufReader::open() {
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

void RingBufReader::close() {
    if (rb) {
        ring_buffer__free(rb);
        rb = nullptr;
    }
    if (map_fd >= 0) {
        ::close(map_fd);
        map_fd = -1;
    }
}

int RingBufReader::handle_ringbuf_event(void *ctx, void *data, size_t size) {
    RingBufReader *reader = static_cast<RingBufReader*>(ctx);
    
    if (size != sizeof(data_t)) {
        Logger::warn("Unexpected data size: " + std::to_string(size) + " expected: " + std::to_string(sizeof(data_t)));
        return 0;
    }
    
    data_t *event_data = static_cast<data_t*>(data);
    
    if (reader->user_callback) {
        reader->user_callback(*event_data);
    }
    
    return 0;
}

void RingBufReader::start_reading(EventCallback callback) {
    if (!rb) {
        throw std::runtime_error("Ring buffer not opened");
    }
    
    if (running) {
        Logger::warn("Ring buffer reader already running");
        return;
    }
    
    user_callback = callback;
    running = true;
    read_thread = std::thread(&RingBufReader::read_loop, this);
    
    Logger::info("Ring buffer reader started");
}

void RingBufReader::stop_reading() {
    if (running) {
        running = false;
        if (read_thread.joinable()) {
            read_thread.join();
        }
        Logger::info("Ring buffer reader stopped");
    }
}

void RingBufReader::read_loop() {
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