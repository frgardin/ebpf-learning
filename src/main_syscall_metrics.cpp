#include <iostream>
#include <thread>
#include <chrono>
#include "BpfSyscallFrequencyReader.hpp"
#include "Logger.hpp"
#include "InfluxClient.hpp"

int main()
{
    Logger::setLogLevel(LogLevel::INFO);

    InfluxClient influxClient("http", "localhost", 8086, "syscall_metrics");

    if (influxClient.ping())
    {
        Logger::info("Successfully connected to InfluxDB");
    }
    else
    {
        Logger::error("Failed to connect to InfluxDB");
        return 1;
    }

    try
    {
        BpfSyscallFrequencyReader bpfReader;

        Logger::info("Starting eBPF syscall monitor. Press Ctrl+C to stop.");

        while (true)
        {
            auto data = bpfReader.readAll();

            Logger::info("=== Syscall Counts ===");
            for (const auto &[key, count] : data)
            {
                std::string syscall_name = bpfReader.getSyscallName(key);
                Logger::info(std::format("{:12}: {}", syscall_name, count));
            }
            Logger::info("=====================");

            // Store in time series for later analysis
            for (const auto &[key, count] : data)
            {
                Logger::addTimeSeriesData(
                    bpfReader.getSyscallName(key),
                    std::to_string(count));
            }

            // Write to InfluxDB
            std::vector<std::string> lines;
            for (const auto &[key, count] : data)
            {
                std::string syscall_name = bpfReader.getSyscallName(key);
                std::string line = std::format("syscall_counts,syscall={} count={}", syscall_name, count);
                lines.push_back(line);
            }
            if (influxClient.writeBatch(lines))
            {
                Logger::info("Successfully wrote syscall counts to InfluxDB");
            }
            else
            {
                Logger::error("Failed to write syscall counts to InfluxDB");
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    catch (const std::exception &e)
    {
        Logger::error(std::format("Error: {}", e.what()));
        return 1;
    }

    return 0;
}