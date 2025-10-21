#include "K8sPerformanceCollector.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>

K8sPerformanceCollector::K8sPerformanceCollector(const std::string& protocol,
                                               const std::string& host, 
                                               int port,
                                               const std::string& database
                                            )
    : influx_(protocol, host, port, database), 
      ring_reader_("/sys/fs/bpf/k8s_perf_events"),
      running_(false),
      last_batch_flush_(std::chrono::steady_clock::now()) {
}

K8sPerformanceCollector::~K8sPerformanceCollector() {
    stop();
}

void K8sPerformanceCollector::start() {
    if (!ring_reader_.open()) {
        Logger::error("Failed to open ring buffer");
        return;
    }
    
    // Create database if it doesn't exist
    if (!influx_.createDatabase("k8s_performance")) {
        Logger::warn("Failed to create database, it might already exist");
    }
    
    running_ = true;
    process_thread_ = std::thread(&K8sPerformanceCollector::process_events, this);
    
    // Start periodic k8s info updates
    std::thread update_thread([this]() {
        while (running_) {
            update_k8s_info_cache();
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    });
    update_thread.detach();
    
    Logger::info("K8s Performance Collector started");
}

void K8sPerformanceCollector::stop() {
    if (running_) {
        running_ = false;
        if (process_thread_.joinable()) {
            process_thread_.join();
        }
        ring_reader_.stop_reading();
        flush_batch(); // Flush any remaining data
        Logger::info("K8s Performance Collector stopped");
    }
}

void K8sPerformanceCollector::process_events() {
    ring_reader_.start_reading([this](const k8s_perf_event& event) {
        // Format the event as InfluxDB line protocol
        std::string metric_line;
        
        switch (event.event_type) {
            case EVENT_SYSCALL_EXIT:
                metric_line = format_syscall_metric(event);
                break;
            case EVENT_NET_RX:
            case EVENT_NET_TX:
                metric_line = format_network_metric(event);
                break;
            default:
                return; // Skip unknown event types
        }
        
        if (!metric_line.empty()) {
            batch_buffer_.push_back(metric_line);
            
            // Check if we should flush the batch
            auto now = std::chrono::steady_clock::now();
            if (batch_buffer_.size() >= max_batch_size_ || 
                (now - last_batch_flush_) > batch_flush_interval_) {
                flush_batch();
            }
        }
    });
}

void K8sPerformanceCollector::flush_batch() {
    if (batch_buffer_.empty()) {
        return;
    }
    
    try {
        if (influx_.writeBatch(batch_buffer_)) {
            Logger::debug("Successfully wrote batch of " + std::to_string(batch_buffer_.size()) + " metrics");
        } else {
            Logger::error("Failed to write batch of " + std::to_string(batch_buffer_.size()) + " metrics");
        }
    } catch (const std::exception& e) {
        Logger::error("Exception while writing batch: " + std::string(e.what()));
    }
    
    batch_buffer_.clear();
    last_batch_flush_ = std::chrono::steady_clock::now();
}

std::string K8sPerformanceCollector::format_syscall_metric(const k8s_perf_event& event) {
    std::stringstream line;
    
    // Measurement
    line << "syscall_latency";
    
    // Tags
    line << ",pod=" << get_pod_info(event.pid);
    line << ",container=" << get_container_info(event.pid);
    line << ",namespace=" << get_namespace_info(event.pid);
    line << ",command=" << event.command;
    line << ",syscall_id=" << event.syscall_id;
    
    // Fields
    line << " ";
    line << "duration_ns=" << event.duration_ns << "i";
    line << ",bytes=" << event.bytes << "i";
    line << ",count=1i";
    
    // Timestamp (nanoseconds)
    //line << " " << event.timestamp;
    
    return line.str();
}

std::string K8sPerformanceCollector::format_network_metric(const k8s_perf_event& event) {
    std::stringstream line;
    std::string direction = (event.event_type == EVENT_NET_RX) ? "rx" : "tx";
    
    // Measurement
    line << "network_events";
    
    // Tags
    line << ",pod=" << get_pod_info(event.pid);
    line << ",container=" << get_container_info(event.pid);
    line << ",namespace=" << get_namespace_info(event.pid);
    line << ",command=" << event.command;
    line << ",direction=" << direction;
    
    // Fields
    line << " ";
    line << "count=1i";
    line << ",bytes=" << event.bytes << "i";
    
    // Timestamp
    //line << " " << event.timestamp;
    
    return line.str();
}

std::string K8sPerformanceCollector::get_pod_info(int pid) {
    // Simple implementation - read from /proc/pid/cgroup
    std::string cgroup_path = "/proc/" + std::to_string(pid) + "/cgroup";
    
    if (!std::filesystem::exists(cgroup_path)) {
        return "unknown";
    }
    
    std::ifstream file(cgroup_path);
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.find("kubepods") != std::string::npos) {
            size_t pod_start = line.find("pod");
            if (pod_start != std::string::npos) {
                size_t container_start = line.find("/", pod_start);
                if (container_start != std::string::npos) {
                    std::string pod_uuid = line.substr(pod_start + 3, container_start - pod_start - 3);
                    return "pod-" + pod_uuid.substr(0, 8);
                }
            }
        }
    }
    
    return "unknown";
}

std::string K8sPerformanceCollector::get_container_info(int pid) {
    // Similar to get_pod_info but extract container ID
    std::string cgroup_path = "/proc/" + std::to_string(pid) + "/cgroup";
    
    if (!std::filesystem::exists(cgroup_path)) {
        return "unknown";
    }
    
    std::ifstream file(cgroup_path);
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.find("kubepods") != std::string::npos) {
            size_t container_start = line.find_last_of("/");
            if (container_start != std::string::npos) {
                std::string container_id = line.substr(container_start + 1);
                return container_id.substr(0, 12); // Docker container ID prefix
            }
        }
    }
    
    return "unknown";
}

std::string K8sPerformanceCollector::get_namespace_info(int pid) {
    // For simplicity, return default
    // In production, you'd query Kubernetes API or read from /proc
    return "default";
}

void K8sPerformanceCollector::update_k8s_info_cache() {
    // Clear and rebuild cache
    pid_to_pod_.clear();
    pid_to_container_.clear();
    pid_to_namespace_.clear();
    
    Logger::debug("Updated K8s info cache");
}