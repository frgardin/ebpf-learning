#include <curl/curl.h>
#include <sstream>
#include <iostream>
#include "InfluxClient.hpp"

InfluxClient::InfluxClient(const std::string &protocol,
                           const std::string &host,
                           int port,
                           const std::string &database)
    : baseUrl_(protocol + "://" + host + ":" + std::to_string(port)), database_(database)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

size_t InfluxClient::WriteCallback(void *contents, size_t size, size_t nmemb, std::string *response)
{
    size_t totalSize = size * nmemb;
    response->append((char *)contents, totalSize);
    return totalSize;
}

bool InfluxClient::writeRaw(const std::string &lineProtocol)
{
    CURL *curl = curl_easy_init();

    if (!curl)
    {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return false;
    }

    std::string response;
    std::string writeUrl = baseUrl_ + "/write?db=" + database_;

    curl_easy_setopt(curl, CURLOPT_URL, writeUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, lineProtocol.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, lineProtocol.length());

    // Set headers
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: text/plain; charset=utf-8");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    return true;
}

bool InfluxClient::write(const std::string &measurement,
                         const std::vector<std::pair<std::string, std::string>> &fields,
                         const std::vector<std::pair<std::string, std::string>> &tags,
                         const std::string &timestamp)
{

    std::stringstream lineProtocol;

    // Measurement
    lineProtocol << measurement;

    // Tags (comma-separated, no spaces)
    for (const auto &tag : tags)
    {
        lineProtocol << "," << tag.first << "=" << tag.second;
    }

    // Space between tags and fields
    lineProtocol << " ";

    // Fields (comma-separated)
    bool firstField = true;
    for (const auto &field : fields)
    {
        if (!firstField)
        {
            lineProtocol << ",";
        }
        lineProtocol << field.first << "=" << field.second;
        firstField = false;
    }

    // Timestamp (optional)
    if (!timestamp.empty())
    {
        lineProtocol << " " << timestamp;
    }

    return writeRaw(lineProtocol.str());
}

bool InfluxClient::writeBatch(const std::vector<std::string> &lines)
{
    std::string batchData;
    for (const auto &line : lines)
    {
        batchData += line + "\n";
    }
    return writeRaw(batchData);
}

bool InfluxClient::ping()
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return false;

    std::string response;
    std::string pingUrl = baseUrl_ + "/ping";

    curl_easy_setopt(curl, CURLOPT_URL, pingUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK);
}

bool InfluxClient::createDatabase(const std::string &dbName)
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return false;

    std::string response;
    std::string queryUrl = baseUrl_ + "/query";
    std::string postData = "q=CREATE DATABASE " + dbName;

    curl_easy_setopt(curl, CURLOPT_URL, queryUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK);
}