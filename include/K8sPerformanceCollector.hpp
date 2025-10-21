#pragma once
#include <string>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <vector>
#include "InfluxClient.hpp"
#include "RingBufReaderK8s.hpp"
#include "Logger.hpp"

class K8sPerformanceCollector {
private:
    InfluxClient influx_;
    RingBufReaderK8s ring_reader_;
    std::atomic<bool> running_;
    std::thread process_thread_;
    
    // Batch processing
    std::vector<std::string> batch_buffer_;
    const size_t max_batch_size_ = 1000;
    std::chrono::steady_clock::time_point last_batch_flush_;
    const std::chrono::milliseconds batch_flush_interval_{5000};
    
    // Kubernetes info cache
    std::unordered_map<int, std::string> pid_to_pod_;
    std::unordered_map<int, std::string> pid_to_container_;
    std::unordered_map<int, std::string> pid_to_namespace_;
    
    void process_events();
    void flush_batch();
    void update_k8s_info_cache();
    std::string get_pod_info(int pid);
    std::string get_container_info(int pid);
    std::string get_namespace_info(int pid);
    
    // Metric formatting
    std::string format_syscall_metric(const k8s_perf_event& event);
    std::string format_network_metric(const k8s_perf_event& event);
    
public:
    K8sPerformanceCollector(const std::string& protocol = "http",
                           const std::string& host = "localhost", 
                           int port = 8086,
                           const std::string& database = "k8s_performance");
    ~K8sPerformanceCollector();
    
    void start();
    void stop();
    bool is_running() const { return running_; }
    
    bool test_connection() { return influx_.ping(); }
};