#include <iostream>
#include <thread>
#include <chrono>
#include "BpfSyscallFrequencyReader.hpp"
#include "Logger.hpp"

int main() {

    Logger::setLogLevel(LogLevel::INFO);
    
    try {
        BpfSyscallFrequencyReader bpfReader;
        
        Logger::info("Starting eBPF syscall monitor. Press Ctrl+C to stop.");
        
        while (true) {
            auto data = bpfReader.readAll();
            
            Logger::info("=== Syscall Counts ===");
            for (const auto& [key, count] : data) {
                std::string syscall_name = bpfReader.getSyscallName(key);
                Logger::info(std::format("{:12}: {}", syscall_name, count));
            }
            Logger::info("=====================");
            
            // Store in time series for later analysis
            for (const auto& [key, count] : data) {
                Logger::addTimeSeriesData(
                    bpfReader.getSyscallName(key), 
                    std::to_string(count)
                );
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        
    } catch (const std::exception& e) {
        Logger::error(std::format("Error: {}", e.what()));
        return 1;
    }
    
    return 0;
}