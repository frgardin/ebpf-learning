#pragma once
#include <string>
#include <vector>
#include "influxdb.hpp"

class InfluxClient
{
private:
    influxdb_cpp::server_info server_info;
    std::string db;

public:
    explicit InfluxClient(const std::string &host,
                          int port,
                          const std::string &db = "",
                          const std::string &usr = "",
                          const std::string &pwd = "",
                          const std::string &precision = "ms",
                          const std::string &token = "");
    std::string getServerUrl() const
    {
        return server_info.host_ + ":" + std::to_string(server_info.port_);
    }
    std::string getDatabase() const
    {
        return db;
    }
    void writeMetric(const std::string &measurement, const std::string &tag, double value);
    void writeMetric(const std::string &measurement,
                     const std::vector<std::pair<std::string, std::string>> &tags,
                     const std::vector<std::pair<std::string, double>> &fields);
};