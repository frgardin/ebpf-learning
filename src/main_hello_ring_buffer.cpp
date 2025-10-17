#include "RingBufReader.hpp"
#include "Logger.hpp"
#include "InfluxClient.hpp"
#include <thread>
#include <chrono>

int main() {
    Logger::setLogLevel(LogLevel::INFO);
    RingBufReader ringBufReader("/sys/fs/bpf/output");
    InfluxClient influxClient("http", "localhost", 8086, "hello_ring_buffer");

    if (!ringBufReader.open()) {
        Logger::error("Failed to open ring buffer reader");
        return 1;
    }

    if (!influxClient.ping()) {
        Logger::error("Failed to connect to InfluxDB");
        return 1;
    }

    auto eventCallback = [&](const data_t& event) {
        Logger::info(std::format("PID: {}, UID: {}, Command: {}, Message: {}",
                                 event.pid, event.uid, event.command, event.message));

        std::string line = std::format("hello_events,pid={},uid={},command={} message=\"{}\"",
                                       event.pid, event.uid, event.command, event.message);
        if (influxClient.writeRaw(line)) {
            Logger::info("Successfully wrote event to InfluxDB");
        } else {
            Logger::error("Failed to write event to InfluxDB");
        }
    };

    ringBufReader.start_reading(eventCallback);

    Logger::info("Started reading from ring buffer. Press Ctrl+C to stop.");

    // Keep the main thread alive while reading
    while (ringBufReader.is_running()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    ringBufReader.stop_reading();
    ringBufReader.close();
    return 0;
}