#include <iostream>
#include <csignal>
#include <atomic>
#include "K8sPerformanceCollector.hpp"
#include "Logger.hpp"

int main(int argc, char *argv[])
{

    Logger::setLogLevel(LogLevel::INFO);

    try
    {
        // Configure from command line or environment variables
        std::string influx_host = "localhost";
        int influx_port = 8086;
        std::string database = "k8s_performance";

        if (argc > 1)
            influx_host = argv[1];
        if (argc > 2)
            influx_port = std::stoi(argv[2]);
        if (argc > 3)
            database = argv[3];

        K8sPerformanceCollector collector("http", influx_host, influx_port, database);

        Logger::info("Starting Kubernetes Performance Monitor");
        Logger::info("InfluxDB: " + influx_host + ":" + std::to_string(influx_port) + "/" + database);

        // Test connection
        if (!collector.test_connection())
        {
            Logger::warn("InfluxDB connection test failed - check if InfluxDB is running");
        }
        else
        {
            Logger::info("InfluxDB connection successful");
        }

        collector.start();

        Logger::info("Kubernetes Performance Monitor started successfully");
        Logger::info("Press Ctrl+C to stop...");

        // Main loop
        while (collector.is_running())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        collector.stop();
        Logger::info("Kubernetes Performance Monitor stopped gracefully");
    }
    catch (const std::exception &e)
    {
        Logger::error("Fatal error: " + std::string(e.what()));
        return 1;
    }

    return 0;
}