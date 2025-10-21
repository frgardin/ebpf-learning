#include "RingBufReaderK8s.hpp"
#include <cstring>
#include <stdexcept>
#include <unistd.h>

RingBufReaderK8s::RingBufReaderK8s(const std::string &cpu_pinned_path, 
                                 const std::string &memory_pinned_path) 
    : cpu_map_path(cpu_pinned_path), 
      memory_map_path(memory_pinned_path), 
      cpu_map_fd(-1), 
      memory_map_fd(-1), 
      cpu_rb(nullptr), 
      memory_rb(nullptr),
      running(false) {
}

RingBufReaderK8s::~RingBufReaderK8s() {
    stop_reading();
    close();
}

bool RingBufReaderK8s::open() {
    // Open CPU ring buffer
    cpu_map_fd = bpf_obj_get(cpu_map_path.c_str());
    if (cpu_map_fd < 0) {
        Logger::error("Failed to open CPU ring buffer map: " + cpu_map_path);
        return false;
    }
    
    cpu_rb = ring_buffer__new(cpu_map_fd, handle_cpu_event, this, nullptr);
    if (!cpu_rb) {
        Logger::error("Failed to create CPU ring buffer");
        ::close(cpu_map_fd);
        cpu_map_fd = -1;
        return false;
    }
    
    // Open Memory ring buffer
    memory_map_fd = bpf_obj_get(memory_map_path.c_str());
    if (memory_map_fd < 0) {
        Logger::error("Failed to open Memory ring buffer map: " + memory_map_path);
        ring_buffer__free(cpu_rb);
        cpu_rb = nullptr;
        ::close(cpu_map_fd);
        cpu_map_fd = -1;
        return false;
    }
    
    memory_rb = ring_buffer__new(memory_map_fd, handle_memory_event, this, nullptr);
    if (!memory_rb) {
        Logger::error("Failed to create Memory ring buffer");
        ring_buffer__free(cpu_rb);
        cpu_rb = nullptr;
        ::close(cpu_map_fd);
        cpu_map_fd = -1;
        ::close(memory_map_fd);
        memory_map_fd = -1;
        return false;
    }
    
    Logger::info("CPU and Memory ring buffers opened successfully");
    return true;
}

void RingBufReaderK8s::close() {
    if (cpu_rb) {
        ring_buffer__free(cpu_rb);
        cpu_rb = nullptr;
    }
    if (memory_rb) {
        ring_buffer__free(memory_rb);
        memory_rb = nullptr;
    }
    if (cpu_map_fd >= 0) {
        ::close(cpu_map_fd);
        cpu_map_fd = -1;
    }
    if (memory_map_fd >= 0) {
        ::close(memory_map_fd);
        memory_map_fd = -1;
    }
}

int RingBufReaderK8s::handle_cpu_event(void *ctx, void *data, size_t size) {
    RingBufReaderK8s *reader = static_cast<RingBufReaderK8s*>(ctx);
    
    if (size != sizeof(cpu_event)) {
        Logger::warn("Unexpected CPU data size: " + std::to_string(size) + 
                    " expected: " + std::to_string(sizeof(cpu_event)));
        return 0;
    }
    
    cpu_event *event_data = static_cast<cpu_event*>(data);
    
    if (reader->cpu_callback) {
        reader->cpu_callback(*event_data);
    }
    
    return 0;
}

int RingBufReaderK8s::handle_memory_event(void *ctx, void *data, size_t size) {
    RingBufReaderK8s *reader = static_cast<RingBufReaderK8s*>(ctx);
    
    if (size != sizeof(memory_event)) {
        Logger::warn("Unexpected Memory data size: " + std::to_string(size) + 
                    " expected: " + std::to_string(sizeof(memory_event)));
        return 0;
    }
    
    memory_event *event_data = static_cast<memory_event*>(data);
    
    if (reader->memory_callback) {
        reader->memory_callback(*event_data);
    }
    
    return 0;
}

void RingBufReaderK8s::start_reading(CpuEventCallback cpu_cb, MemoryEventCallback memory_cb) {
    if (!cpu_rb || !memory_rb) {
        throw std::runtime_error("Ring buffers not opened");
    }
    
    if (running) {
        Logger::warn("Ring buffer reader already running");
        return;
    }
    
    cpu_callback = cpu_cb;
    memory_callback = memory_cb;
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
        // Poll CPU ring buffer
        int err = ring_buffer__poll(cpu_rb, timeout_ms);
        if (err < 0 && err != -EINTR) {
            Logger::error("Error polling CPU ring buffer: " + std::to_string(err));
        }
        
        // Poll Memory ring buffer
        err = ring_buffer__poll(memory_rb, timeout_ms);
        if (err < 0 && err != -EINTR) {
            Logger::error("Error polling Memory ring buffer: " + std::to_string(err));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}