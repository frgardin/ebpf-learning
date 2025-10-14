#pragma once
#include <InfluxDBFactory.h>
#include <memory>
#include <string>

class InfluxClient {
private:
    std::unique_ptr<influxdb::InfluxDB> client;

public:
    explicit InfluxClient(const std::string& url);  
    void writeMetric(const std::string& measurement, const std::string& tag, double value);
};
