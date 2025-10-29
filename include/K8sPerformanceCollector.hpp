#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include "InfluxClient.hpp"
#include "RingBufReaderK8s.hpp"
#include "Logger.hpp"

class K8sPerformanceCollector
{
private:
    InfluxClient influx_;
    RingBufReaderK8s ring_reader_;
    std::atomic<bool> running_;
    std::thread process_thread_;

    // Batch processing
    std::vector<std::string> batch_buffer_;
    std::chrono::steady_clock::time_point last_batch_flush_;
    static const size_t max_batch_size_ = 1000;
    static const std::chrono::seconds batch_flush_interval_;

    // Pod tracking
    std::unordered_map<__u32, std::string> pid_to_pod_;
    std::unordered_map<__u32, std::string> pid_to_container_;
    std::unordered_map<__u32, std::string> pid_to_namespace_;

    // Metrics aggregation
    std::unordered_map<std::string, std::unordered_map<std::string, double>> pod_metrics_;

    // IO-specific tracking
    std::unordered_map<std::string, std::unordered_map<std::string, double>> io_pod_metrics_;

public:
    K8sPerformanceCollector(const std::string &protocol = "http",
                            const std::string &host = "localhost",
                            int port = 8086,
                            const std::string &database = "k8s_metrics");
    ~K8sPerformanceCollector();

    void start();
    void stop();
    bool is_running() const { return running_; }

    bool test_connection() { return influx_.ping(); }

private:
    void process_events();
    void flush_batch();

    // Event handlers
    void handle_cpu_event(const cpu_event &event);
    void handle_memory_event(const memory_event &event);
    void handle_syscall_latency_event(const syscall_latency_event &event);

    // Metric formatting
    std::string format_cpu_metric(const cpu_event &event, const std::string &pod_info);
    std::string format_memory_metric(const memory_event &event, const std::string &pod_info);
    std::string format_syscall_latency_metric(const syscall_latency_event &event, const std::string &pod_info);

    // Kubernetes info extraction
    std::string get_pod_info(__u32 pid);
    std::string get_container_info(__u32 pid);
    std::string get_namespace_info(__u32 pid);
    void update_k8s_info_cache();
    std::string extract_pod_from_cgroup(const std::string &cgroup_line);
    std::string extract_container_from_cgroup(const std::string &cgroup_line);

    // Metrics aggregation
    void update_pod_metrics(const std::string &pod_name, const std::string &metric_name, double value);
    void update_io_pod_metrics(const std::string &pod_name, const std::string &metric_name, double value);
    void flush_aggregated_metrics();

    // Syscall utilities
    std::string get_syscall_name(int syscall_id);
    bool is_io_syscall(int syscall_id);
    bool is_important_syscall(int syscall_id);
};