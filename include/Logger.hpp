#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <vector>

enum class LogLevel
{
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class Logger
{
private:
    static std::mutex log_mutex_;
    static LogLevel current_level_;
    static std::unique_ptr<std::ofstream> file_stream_;
    static bool console_output_;
    static bool file_output_;
    static std::string log_filename_;

    // Time series data storage
    static std::unordered_map<std::string, std::vector<std::pair<std::chrono::system_clock::time_point, std::string>>> time_series_data_;

    static std::string getCurrentTimestamp();
    static std::string levelToString(LogLevel level);
    static void writeLog(LogLevel level, const std::string &message, const std::string &time_series_key = "");

public:
    // Configuration methods
    static void setLogLevel(LogLevel level);
    static void enableConsoleOutput(bool enable);
    static void enableFileOutput(bool enable, const std::string &filename = "");
    static void setLogFile(const std::string &filename);

    // Logging methods
    static void trace(const std::string &message, const std::string &time_series_key = "");
    static void debug(const std::string &message, const std::string &time_series_key = "");
    static void info(const std::string &message, const std::string &time_series_key = "");
    static void warn(const std::string &message, const std::string &time_series_key = "");
    static void error(const std::string &message, const std::string &time_series_key = "");
    static void fatal(const std::string &message, const std::string &time_series_key = "");

    // Time series methods
    static void addTimeSeriesData(const std::string &key, const std::string &value);
    static std::vector<std::pair<std::chrono::system_clock::time_point, std::string>>
    getTimeSeriesData(const std::string &key);
    static void clearTimeSeriesData(const std::string &key = "");
    static void exportTimeSeriesToCSV(const std::string &filename, const std::string &key = "");

    // Utility methods
    static void flush();
    static std::string getLogLevelString();

    // Structured logging
    template <typename... Args>
    static void trace(const std::string &format, Args... args)
    {
        logFormatted(LogLevel::TRACE, format, args...);
    }

    template <typename... Args>
    static void debug(const std::string &format, Args... args)
    {
        logFormatted(LogLevel::DEBUG, format, args...);
    }

    template <typename... Args>
    static void info(const std::string &format, Args... args)
    {
        logFormatted(LogLevel::INFO, format, args...);
    }

    template <typename... Args>
    static void warn(const std::string &format, Args... args)
    {
        logFormatted(LogLevel::WARN, format, args...);
    }

    template <typename... Args>
    static void error(const std::string &format, Args... args)
    {
        logFormatted(LogLevel::ERROR, format, args...);
    }

    template <typename... Args>
    static void fatal(const std::string &format, Args... args)
    {
        logFormatted(LogLevel::FATAL, format, args...);
    }

private:
    template <typename... Args>
    static void logFormatted(LogLevel level, const std::string &format, Args... args)
    {
        if (level < current_level_)
            return;

        std::ostringstream ss;
        formatMessage(ss, format, args...);
        writeLog(level, ss.str());
    }

    template <typename T, typename... Args>
    static void formatMessage(std::ostringstream &ss, const std::string &format, T value, Args... args)
    {
        size_t pos = format.find("{}");
        if (pos == std::string::npos)
        {
            ss << format;
            return;
        }
        ss << format.substr(0, pos) << value;
        formatMessage(ss, format.substr(pos + 2), args...);
    }

    static void formatMessage(std::ostringstream &ss, const std::string &format)
    {
        ss << format;
    }
};