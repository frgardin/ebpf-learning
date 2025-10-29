#include "K8sPerformanceCollector.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <regex>
#include <unordered_set>

// Initialize static member
const std::chrono::seconds K8sPerformanceCollector::batch_flush_interval_(10);

K8sPerformanceCollector::K8sPerformanceCollector(const std::string &protocol,
                                                 const std::string &host,
                                                 int port,
                                                 const std::string &database)
    : influx_(protocol, host, port, database),
      ring_reader_("/sys/fs/bpf/cpu_events", "/sys/fs/bpf/memory_events", "/sys/fs/bpf/syscall_latency_events"),
      running_(false),
      last_batch_flush_(std::chrono::steady_clock::now())
{
}

K8sPerformanceCollector::~K8sPerformanceCollector()
{
    stop();
}

void K8sPerformanceCollector::start()
{
    if (!ring_reader_.open())
    {
        Logger::error("Failed to open ring buffers");
        return;
    }

    // Create database if it doesn't exist
    if (!influx_.createDatabase("k8s_performance"))
    {
        Logger::warn("Failed to create database, it might already exist");
    }

    running_ = true;

    // Start processing events
    process_thread_ = std::thread(&K8sPerformanceCollector::process_events, this);

    // Start periodic k8s info updates
    std::thread update_thread([this]()
                              {
        while (running_) {
            update_k8s_info_cache();
            flush_aggregated_metrics();
            std::this_thread::sleep_for(std::chrono::seconds(30));
        } });
    update_thread.detach();

    Logger::info("K8s Performance Collector started");
}

void K8sPerformanceCollector::stop()
{
    if (running_)
    {
        running_ = false;
        if (process_thread_.joinable())
        {
            process_thread_.join();
        }
        ring_reader_.stop_reading();
        flush_batch();              // Flush any remaining data
        flush_aggregated_metrics(); // Flush final aggregated metrics
        Logger::info("K8s Performance Collector stopped");
    }
}

void K8sPerformanceCollector::process_events()
{
    ring_reader_.start_reading(
        [this](const cpu_event &event)
        {
            handle_cpu_event(event);
        },
        [this](const memory_event &event)
        {
            handle_memory_event(event);
        },
        [this](const syscall_latency_event &event)
        {
            handle_syscall_latency_event(event);
        });
}

void K8sPerformanceCollector::handle_cpu_event(const cpu_event &event)
{
    Logger::debug("CPU Event received - PID: " + std::to_string(event.pid) +
                  ", TGID: " + std::to_string(event.tgid) +
                  ", Runtime: " + std::to_string(event.runtime_ns) + "ns");
    std::string pod_info = get_pod_info(event.tgid);
    Logger::debug("Pod info for PID " + std::to_string(event.tgid) + ": " + pod_info);

    if (pod_info != "unknown" || true)
    {
        std::string metric_line = format_cpu_metric(event, pod_info);
        if (!metric_line.empty())
        {
            batch_buffer_.push_back(metric_line);
            Logger::debug("Added CPU metric to batch. Batch size: " + std::to_string(batch_buffer_.size()));

            // Update aggregated metrics
            update_pod_metrics(pod_info, "cpu_time_ns", event.runtime_ns);
            update_pod_metrics(pod_info, "cpu_usage", event.runtime_ns / 1000000.0); // Convert to ms

            // Check if we should flush the batch
            auto now = std::chrono::steady_clock::now();
            if (batch_buffer_.size() >= max_batch_size_ ||
                (now - last_batch_flush_) > batch_flush_interval_)
            {
                flush_batch();
            }
        }
        else
        {
            Logger::debug("Skipping CPU event for unknown pod (PID: " + std::to_string(event.tgid) + ")");
        }
    }
}

void K8sPerformanceCollector::handle_memory_event(const memory_event &event)
{
    Logger::debug("Memory Event received - PID: " + std::to_string(event.pid) +
                  ", TGID: " + std::to_string(event.tgid) +
                  ", RSS: " + std::to_string(event.rss_kb) + "KB");
    std::string pod_info = get_pod_info(event.tgid);

    if (pod_info != "unknown" || true)
    {
        std::string metric_line = format_memory_metric(event, pod_info);
        if (!metric_line.empty())
        {
            batch_buffer_.push_back(metric_line);
            Logger::debug("Added Memory metric to batch. Batch size: " + std::to_string(batch_buffer_.size()));

            // Update aggregated metrics based on event type
            switch (event.event_type)
            {
            case EVENT_MEMORY_ALLOC:
                Logger::debug("Memory EVENT_MEMORY_ALLOC");
                update_pod_metrics(pod_info, "memory_alloc_kb", event.rss_kb);
                break;
            case EVENT_MEMORY_FREE:
                Logger::debug("Memory EVENT_MEMORY_FREE");
                update_pod_metrics(pod_info, "memory_free_kb", event.rss_kb);
                break;
            case EVENT_MEMORY_REPORT:
                Logger::debug("Memory EVENT_MEMORY_REPORT");
                update_pod_metrics(pod_info, "memory_rss_kb", event.rss_kb);
                update_pod_metrics(pod_info, "memory_cache_kb", event.cache_kb);
                break;
            }

            // Check if we should flush the batch
            auto now = std::chrono::steady_clock::now();
            if (batch_buffer_.size() >= max_batch_size_ ||
                (now - last_batch_flush_) > batch_flush_interval_)
            {
                flush_batch();
            }
        }
        else
        {
            Logger::debug("Skipping Memory event for unknown pod (PID: " + std::to_string(event.tgid) + ")");
        }
    }
}

void K8sPerformanceCollector::handle_syscall_latency_event(const syscall_latency_event &event)
{
    Logger::debug("Syscall Latency Event received - PID: " + std::to_string(event.pid) +
                  ", TGID: " + std::to_string(event.tgid) +
                  ", Syscall: " + std::to_string(event.syscall_id) +
                  ", Latency: " + std::to_string(event.runtime_ns) + "ns");

    std::string pod_info = get_pod_info(event.tgid);

    if (pod_info != "unknown" || true)
    {
        // Only process important syscalls to reduce noise
        if (is_important_syscall(event.syscall_id))
        {
            std::string metric_line = format_syscall_latency_metric(event, pod_info);
            if (!metric_line.empty())
            {
                batch_buffer_.push_back(metric_line);
                Logger::debug("Added Syscall Latency metric to batch. Batch size: " + std::to_string(batch_buffer_.size()));

                // Update aggregated metrics
                std::string syscall_name = get_syscall_name(event.syscall_id);

                // General syscall metrics
                update_pod_metrics(pod_info, "syscall_latency_ns", event.runtime_ns);
                update_pod_metrics(pod_info, "syscall_count", 1);
                update_pod_metrics(pod_info, "syscall_" + syscall_name + "_latency_ns", event.runtime_ns);
                update_pod_metrics(pod_info, "syscall_" + syscall_name + "_count", 1);

                // IO-specific metrics
                if (is_io_syscall(event.syscall_id))
                {
                    update_io_pod_metrics(pod_info, "io_latency_ns", event.runtime_ns);
                    update_io_pod_metrics(pod_info, "io_ops_count", 1);
                    update_io_pod_metrics(pod_info, "io_" + syscall_name + "_latency_ns", event.runtime_ns);
                    update_io_pod_metrics(pod_info, "io_" + syscall_name + "_count", 1);
                }

                // Check if we should flush the batch
                auto now = std::chrono::steady_clock::now();
                if (batch_buffer_.size() >= max_batch_size_ ||
                    (now - last_batch_flush_) > batch_flush_interval_)
                {
                    flush_batch();
                }
            }
        }
        else
        {
            Logger::debug("Skipping non-important syscall: " + std::to_string(event.syscall_id));
        }
    }
    else
    {
        Logger::debug("Skipping Syscall Latency event for unknown pod (PID: " + std::to_string(event.tgid) + ")");
    }
}

std::string K8sPerformanceCollector::format_cpu_metric(const cpu_event &event, const std::string &pod_info)
{
    std::stringstream line;

    line << "cpu_usage";
    line << ",pod=" << pod_info;
    line << ",container=" << get_container_info(event.tgid);
    line << ",namespace=" << get_namespace_info(event.tgid);
    line << ",command=" << event.comm;
    line << ",cpu_id=" << event.cpu_id;
    line << ",pid=" << event.pid;
    line << " ";
    line << "runtime_ns=" << event.runtime_ns << "i";
    line << ",usage_percent=" << (event.runtime_ns / 10000000.0); // Simplified calculation

    // Add current timestamp in nanoseconds
    auto now = std::chrono::system_clock::now();
    auto timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            now.time_since_epoch())
                            .count();
    line << " " << timestamp_ns;

    return line.str();
}

std::string K8sPerformanceCollector::format_memory_metric(const memory_event &event, const std::string &pod_info)
{
    std::stringstream line;

    std::string event_type_str;
    switch (event.event_type)
    {
    case EVENT_MEMORY_ALLOC:
        event_type_str = "alloc";
        break;
    case EVENT_MEMORY_FREE:
        event_type_str = "free";
        break;
    case EVENT_MEMORY_REPORT:
        event_type_str = "report";
        break;
    default:
        event_type_str = "unknown";
    }

    line << "memory_usage";
    line << ",pod=" << pod_info;
    line << ",container=" << get_container_info(event.tgid);
    line << ",namespace=" << get_namespace_info(event.tgid);
    line << ",command=" << event.comm;
    line << ",event_type=" << event_type_str;
    line << " ";
    line << "rss_kb=" << event.rss_kb << "i";
    line << ",cache_kb=" << event.cache_kb << "i";

    // Add current timestamp in nanoseconds
    auto now = std::chrono::system_clock::now();
    auto timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            now.time_since_epoch())
                            .count();
    line << " " << timestamp_ns;

    return line.str();
}

std::string K8sPerformanceCollector::format_syscall_latency_metric(const syscall_latency_event &event, const std::string &pod_info)
{
    std::stringstream line;

    std::string syscall_name = get_syscall_name(event.syscall_id);
    bool is_io = is_io_syscall(event.syscall_id);

    line << "syscall_latency";
    line << ",pod=" << pod_info;
    line << ",container=" << get_container_info(event.tgid);
    line << ",namespace=" << get_namespace_info(event.tgid);
    line << ",command=" << event.comm;
    line << ",syscall=" << syscall_name;
    line << ",syscall_id=" << event.syscall_id;
    line << ",is_io=" << (is_io ? "true" : "false");
    line << " ";
    line << "latency_ns=" << event.runtime_ns << "i";
    line << ",latency_us=" << (event.runtime_ns / 1000.0);
    line << ",latency_ms=" << (event.runtime_ns / 1000000.0);

    // Add current timestamp in nanoseconds
    auto now = std::chrono::system_clock::now();
    auto timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            now.time_since_epoch())
                            .count();
    line << " " << timestamp_ns;

    return line.str();
}

void K8sPerformanceCollector::flush_batch()
{
    if (batch_buffer_.empty())
    {
        return;
    }

    try
    {
        if (influx_.writeBatch(batch_buffer_))
        {
            Logger::debug("Successfully wrote batch of " + std::to_string(batch_buffer_.size()) + " metrics");
        }
        else
        {
            Logger::error("Failed to write batch of " + std::to_string(batch_buffer_.size()) + " metrics");
        }
    }
    catch (const std::exception &e)
    {
        Logger::error("Exception while writing batch: " + std::string(e.what()));
    }

    batch_buffer_.clear();
    last_batch_flush_ = std::chrono::steady_clock::now();
}

void K8sPerformanceCollector::update_pod_metrics(const std::string &pod_name, const std::string &metric_name, double value)
{
    pod_metrics_[pod_name][metric_name] += value;
}

void K8sPerformanceCollector::update_io_pod_metrics(const std::string &pod_name, const std::string &metric_name, double value)
{
    io_pod_metrics_[pod_name][metric_name] += value;
}

void K8sPerformanceCollector::flush_aggregated_metrics()
{
    std::vector<std::string> aggregated_batch;

    // Flush general pod metrics
    for (const auto &[pod_name, metrics] : pod_metrics_)
    {
        for (const auto &[metric_name, value] : metrics)
        {
            std::stringstream line;
            line << "pod_aggregated";
            line << ",pod=" << pod_name;
            line << ",metric=" << metric_name;
            line << " ";
            line << "value=" << value;

            // Add current timestamp in nanoseconds
            auto now = std::chrono::system_clock::now();
            auto timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                    now.time_since_epoch())
                                    .count();
            line << " " << timestamp_ns;

            aggregated_batch.push_back(line.str());
        }
    }

    // Flush IO-specific pod metrics
    for (const auto &[pod_name, metrics] : io_pod_metrics_)
    {
        for (const auto &[metric_name, value] : metrics)
        {
            std::stringstream line;
            line << "io_aggregated";
            line << ",pod=" << pod_name;
            line << ",metric=" << metric_name;
            line << " ";
            line << "value=" << value;

            // Add current timestamp in nanoseconds
            auto now = std::chrono::system_clock::now();
            auto timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                    now.time_since_epoch())
                                    .count();
            line << " " << timestamp_ns;

            aggregated_batch.push_back(line.str());
        }
    }

    if (!aggregated_batch.empty())
    {
        if (influx_.writeBatch(aggregated_batch))
        {
            Logger::debug("Successfully wrote " + std::to_string(aggregated_batch.size()) + " aggregated metrics");
        }
        else
        {
            Logger::error("Failed to write aggregated metrics batch");
        }
    }

    // Clear aggregated metrics after flushing
    pod_metrics_.clear();
    io_pod_metrics_.clear();
}

std::string K8sPerformanceCollector::get_pod_info(__u32 pid)
{
    Logger::debug("=== get_pod_info called for PID: " + std::to_string(pid) + " ===");

    std::string cgroup_path = "/proc/" + std::to_string(pid) + "/cgroup";
    Logger::debug("Reading cgroup from: " + cgroup_path);

    // Check if file exists
    if (!std::filesystem::exists(cgroup_path))
    {
        Logger::debug("Cgroup file does not exist for PID: " + std::to_string(pid));
        return "unknown";
    }

    try
    {
        std::ifstream file(cgroup_path);
        if (!file.is_open())
        {
            Logger::debug("Failed to open cgroup file for PID: " + std::to_string(pid));
            return "unknown";
        }

        std::string line;
        int line_num = 0;

        while (std::getline(file, line))
        {
            line_num++;
            Logger::debug("Cgroup line " + std::to_string(line_num) + ": " + line);

            // Check for Kubernetes patterns
            if (line.find("kubepods") != std::string::npos ||
                line.find("docker") != std::string::npos ||
                line.find("containerd") != std::string::npos)
            {

                std::string pod_info = extract_pod_from_cgroup(line);
                Logger::debug("Found container runtime, extracted: " + pod_info);

                if (pod_info != "unknown")
                {
                    pid_to_pod_[pid] = pod_info;
                    return pod_info;
                }
            }
        }

        Logger::debug("No container patterns found in cgroup for PID: " + std::to_string(pid));
    }
    catch (const std::exception &e)
    {
        Logger::error("Exception reading cgroup for PID " + std::to_string(pid) + ": " + e.what());
    }

    return "unknown";
}

std::string K8sPerformanceCollector::extract_pod_from_cgroup(const std::string &cgroup_line)
{
    Logger::debug("Analyzing cgroup line: " + cgroup_line);

    // Multiple patterns to match different Kubernetes cgroup formats
    std::vector<std::regex> patterns = {
        std::regex("pod([a-f0-9_-]+)\\.slice"),                                          // systemd with slices (supports _ and -)
        std::regex("pod([a-f0-9_-]+)/"),                                                 // cgroupfs format (supports _ and -)
        std::regex("kubepods[^/]*/pod([a-f0-9_-]+)"),                                    // kubepods prefix (supports _ and -)
        std::regex("kubepods[^/]*-pod([a-f0-9_-]+)\\.slice"),                            // kubepods with slice (supports _ and -)
        std::regex("pod([a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12})"), // full UUID with dashes
        std::regex("pod([a-f0-9]{8}_[a-f0-9]{4}_[a-f0-9]{4}_[a-f0-9]{4}_[a-f0-9]{12})")  // full UUID with underscores
    };

    for (const auto &pattern : patterns)
    {
        std::smatch match;
        if (std::regex_search(cgroup_line, match, pattern) && match.size() > 1)
        {
            std::string pod_uuid = match[1].str();
            Logger::debug("Found pod with pattern, UUID: " + pod_uuid);
            return "pod-" + pod_uuid.substr(0, 8);
        }
    }

    // Check if it's a Docker container but not in Kubernetes
    if (cgroup_line.find("docker") != std::string::npos)
    {
        Logger::debug("Found Docker container but not in Kubernetes");
        return "docker-non-k8s";
    }

    Logger::debug("No Kubernetes pod pattern found in cgroup");
    return "unknown";
}

std::string K8sPerformanceCollector::get_container_info(__u32 pid)
{
    auto it = pid_to_container_.find(pid);
    if (it != pid_to_container_.end())
    {
        return it->second;
    }

    // Extract from cgroup (simplified)
    return "container-" + std::to_string(pid);
}

std::string K8sPerformanceCollector::get_namespace_info(__u32 pid)
{
    auto it = pid_to_namespace_.find(pid);
    if (it != pid_to_namespace_.end())
    {
        return it->second;
    }

    // Default namespace
    return "default";
}

void K8sPerformanceCollector::update_k8s_info_cache()
{
    // Clear old cache
    pid_to_pod_.clear();
    pid_to_container_.clear();
    pid_to_namespace_.clear();

    // Rebuild cache by scanning /proc
    for (const auto &entry : std::filesystem::directory_iterator("/proc"))
    {
        if (entry.is_directory())
        {
            try
            {
                std::string pid_str = entry.path().filename();
                __u32 pid = std::stoi(pid_str);

                // This will populate the cache
                get_pod_info(pid);
            }
            catch (...)
            {
                // Skip non-numeric directory names
            }
        }
    }

    Logger::debug("Updated K8s info cache, tracking " + std::to_string(pid_to_pod_.size()) + " pods");
}

// Syscall utilities
std::string K8sPerformanceCollector::get_syscall_name(int syscall_id)
{
    static std::unordered_map<int, std::string> syscall_names = {
        {0, "read"}, {1, "write"}, {2, "open"}, {3, "close"}, {4, "stat"}, {5, "fstat"}, {6, "lstat"}, {7, "poll"}, {8, "lseek"}, {9, "mmap"}, {10, "mprotect"}, {11, "munmap"}, {12, "brk"}, {13, "rt_sigaction"}, {14, "rt_sigprocmask"}, {16, "ioctl"}, {17, "pread64"}, {18, "pwrite64"}, {19, "readv"}, {20, "writev"}, {21, "access"}, {22, "pipe"}, {23, "select"}, {39, "getpid"}, {56, "clone"}, {57, "fork"}, {58, "vfork"}, {59, "execve"}, {60, "exit"}, {61, "wait4"}, {62, "kill"}, {72, "fcntl"}, {78, "getdents"}, {79, "getcwd"}, {80, "chdir"}, {82, "rename"}, {83, "mkdir"}, {84, "rmdir"}, {85, "creat"}, {86, "link"}, {87, "unlink"}, {88, "symlink"}, {89, "readlink"}, {90, "chmod"}, {92, "chown"}, {102, "getuid"}, {104, "getgid"}, {107, "geteuid"}, {108, "getegid"}, {158, "arch_prctl"}, {160, "setrlimit"}, {161, "chroot"}, {186, "gettid"}, {202, "futex"}, {218, "set_tid_address"}, {228, "clock_gettime"}, {231, "exit_group"}, {232, "epoll_wait"}, {233, "epoll_ctl"}, {234, "tgkill"}, {257, "openat"}, {262, "newfstatat"}, {263, "unlinkat"}, {264, "renameat"}, {268, "fchmodat"}, {269, "faccessat"}, {270, "pselect6"}, {288, "accept4"}, {291, "epoll_create1"}, {292, "dup3"}, {293, "pipe2"}, {302, "prlimit64"}};

    auto it = syscall_names.find(syscall_id);
    if (it != syscall_names.end())
    {
        return it->second;
    }

    return "syscall_" + std::to_string(syscall_id);
}

bool K8sPerformanceCollector::is_io_syscall(int syscall_id)
{
    // Common IO-related syscalls
    static std::unordered_set<int> io_syscalls = {
        0,   // read
        1,   // write
        2,   // open
        3,   // close
        4,   // stat
        5,   // fstat
        6,   // lstat
        8,   // lseek
        9,   // mmap
        10,  // mprotect
        11,  // munmap
        16,  // ioctl
        17,  // pread64
        18,  // pwrite64
        19,  // readv
        20,  // writev
        21,  // access
        72,  // fcntl
        78,  // getdents
        79,  // getcwd
        82,  // rename
        83,  // mkdir
        84,  // rmdir
        85,  // creat
        86,  // link
        87,  // unlink
        88,  // symlink
        89,  // readlink
        90,  // chmod
        92,  // chown
        257, // openat
        262, // newfstatat
        263, // unlinkat
        264, // renameat
        268, // fchmodat
        269  // faccessat
    };

    return io_syscalls.find(syscall_id) != io_syscalls.end();
}

bool K8sPerformanceCollector::is_important_syscall(int syscall_id)
{
    // Filter to only track important syscalls to reduce noise
    static std::unordered_set<int> important_syscalls = {
        0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 16, 17, 18, 19, 20,
        21, 39, 56, 57, 58, 59, 60, 61, 62, 72, 78, 79, 80, 82,
        83, 84, 85, 86, 87, 88, 89, 90, 92, 102, 104, 107, 108,
        158, 160, 161, 186, 202, 218, 228, 231, 232, 233, 234,
        257, 262, 263, 264, 268, 269, 270, 288, 291, 292, 293, 302};

    return important_syscalls.find(syscall_id) != important_syscalls.end();
}