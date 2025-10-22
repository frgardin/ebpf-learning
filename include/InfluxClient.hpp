#pragma once
#include <string>
#include <vector>

class InfluxClient
{
private:
    std::string baseUrl_;
    std::string database_;

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *response);

public:
    InfluxClient(const std::string &protocol = "http",
                 const std::string &host = "localhost",
                 int port = 8086,
                 const std::string &database = "mydb");

    // Write single data point
    bool write(const std::string &measurement,
               const std::vector<std::pair<std::string, std::string>> &fields,
               const std::vector<std::pair<std::string, std::string>> &tags = {},
               const std::string &timestamp = "");

    // Write raw line protocol
    bool writeRaw(const std::string &lineProtocol);

    // Batch write multiple lines
    bool writeBatch(const std::vector<std::string> &lines);

    // Health check
    bool ping();

    // Create database
    bool createDatabase(const std::string &dbName);
};