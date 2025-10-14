#include "InfluxClient.hpp"

InfluxClient::InfluxClient(const std::string& url) {
    client = influxdb::InfluxDBFactory::Get(url);
}

void InfluxClient::writeMetric(const std::string& measurement, const std::string& tag, double value) {
    client->write(influxdb::Point{measurement}
        .addTag("type", tag)
        .addField("value", value)
    );
}
