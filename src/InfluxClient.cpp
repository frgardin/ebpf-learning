#include "InfluxClient.hpp"
#include <stdexcept>
#include <chrono>
#include "influxdb.hpp"

InfluxClient::InfluxClient(const std::string &host,
                           int port,
                           const std::string &db,
                           const std::string &usr,
                           const std::string &pwd,
                           const std::string &precision,
                           const std::string &token)
    : server_info(host, port, db, usr, pwd, precision, token), db(db)
{
    // Validate required parameters
    if (host.empty())
    {
        throw std::invalid_argument("Host cannot be empty");
    }
    if (port <= 0 || port > 65535)
    {
        throw std::invalid_argument("Port must be between 1 and 65535");
    }
}

void InfluxClient::writeMetric(const std::string &measurement, const std::string &tag, double value)
{
}

void InfluxClient::writeMetric(const std::string &measurement,
                               const std::vector<std::pair<std::string, std::string>> &tags,
                               const std::vector<std::pair<std::string, double>> &fields)
{
}