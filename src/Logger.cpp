#include "Logger.hpp"
#include <iostream>
#include <filesystem>

// Initialize static members
std::mutex Logger::log_mutex_;
LogLevel Logger::current_level_ = LogLevel::INFO;
std::unique_ptr<std::ofstream> Logger::file_stream_ = nullptr;
bool Logger::console_output_ = true;
bool Logger::file_output_ = false;
std::string Logger::log_filename_ = "";
std::unordered_map<std::string, std::vector<std::pair<std::chrono::system_clock::time_point, std::string>>> Logger::time_series_data_;

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

void Logger::writeLog(LogLevel level, const std::string& message, const std::string& time_series_key) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    std::string timestamp = getCurrentTimestamp();
    std::string level_str = levelToString(level);
    std::string log_entry = "[" + timestamp + "] [" + level_str + "] " + message;
    
    // Console output
    if (console_output_) {
        if (level >= LogLevel::ERROR) {
            std::cerr << log_entry << std::endl;
        } else {
            std::cout << log_entry << std::endl;
        }
    }
    
    // File output
    if (file_output_ && file_stream_ && file_stream_->is_open()) {
        *file_stream_ << log_entry << std::endl;
    }
    
    // Store in time series if key provided
    if (!time_series_key.empty()) {
        auto now = std::chrono::system_clock::now();
        time_series_data_[time_series_key].emplace_back(now, message);
    }
}

// Configuration methods
void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    current_level_ = level;
}

void Logger::enableConsoleOutput(bool enable) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    console_output_ = enable;
}

void Logger::enableFileOutput(bool enable, const std::string& filename) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    file_output_ = enable;
    
    if (enable && !filename.empty()) {
        setLogFile(filename);
    } else if (!enable && file_stream_) {
        file_stream_->close();
        file_stream_.reset();
    }
}

void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    if (file_stream_ && file_stream_->is_open()) {
        file_stream_->close();
    }
    
    log_filename_ = filename;
    file_stream_ = std::make_unique<std::ofstream>(filename, std::ios::app);
    
    if (!file_stream_->is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
        file_output_ = false;
    } else {
        file_output_ = true;
        // Write header for new file
        *file_stream_ << "[" << getCurrentTimestamp() << "] [INFO] Log file opened" << std::endl;
    }
}

// Logging methods
void Logger::trace(const std::string& message, const std::string& time_series_key) {
    if (LogLevel::TRACE < current_level_) return;
    writeLog(LogLevel::TRACE, message, time_series_key);
}

void Logger::debug(const std::string& message, const std::string& time_series_key) {
    if (LogLevel::DEBUG < current_level_) return;
    writeLog(LogLevel::DEBUG, message, time_series_key);
}

void Logger::info(const std::string& message, const std::string& time_series_key) {
    if (LogLevel::INFO < current_level_) return;
    writeLog(LogLevel::INFO, message, time_series_key);
}

void Logger::warn(const std::string& message, const std::string& time_series_key) {
    if (LogLevel::WARN < current_level_) return;
    writeLog(LogLevel::WARN, message, time_series_key);
}

void Logger::error(const std::string& message, const std::string& time_series_key) {
    if (LogLevel::ERROR < current_level_) return;
    writeLog(LogLevel::ERROR, message, time_series_key);
}

void Logger::fatal(const std::string& message, const std::string& time_series_key) {
    writeLog(LogLevel::FATAL, message, time_series_key);
}

// Time series methods
void Logger::addTimeSeriesData(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    auto now = std::chrono::system_clock::now();
    time_series_data_[key].emplace_back(now, value);
}

std::vector<std::pair<std::chrono::system_clock::time_point, std::string>> 
Logger::getTimeSeriesData(const std::string& key) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    auto it = time_series_data_.find(key);
    if (it != time_series_data_.end()) {
        return it->second;
    }
    return {};
}

void Logger::clearTimeSeriesData(const std::string& key) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    if (key.empty()) {
        time_series_data_.clear();
    } else {
        time_series_data_.erase(key);
    }
}

void Logger::exportTimeSeriesToCSV(const std::string& filename, const std::string& key) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    std::ofstream csv_file(filename);
    if (!csv_file.is_open()) {
        error("Failed to open CSV file: " + filename);
        return;
    }
    
    csv_file << "timestamp,key,value\n";
    
    if (key.empty()) {
        // Export all time series data
        for (const auto& [series_key, data_points] : time_series_data_) {
            for (const auto& [timestamp, value] : data_points) {
                auto time_t = std::chrono::system_clock::to_time_t(timestamp);
                csv_file << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << ","
                         << series_key << "," << value << "\n";
            }
        }
    } else {
        // Export specific time series
        auto it = time_series_data_.find(key);
        if (it != time_series_data_.end()) {
            for (const auto& [timestamp, value] : it->second) {
                auto time_t = std::chrono::system_clock::to_time_t(timestamp);
                csv_file << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << ","
                         << key << "," << value << "\n";
            }
        }
    }
    
    csv_file.close();
    info("Time series data exported to: " + filename);
}

// Utility methods
void Logger::flush() {
    std::lock_guard<std::mutex> lock(log_mutex_);
    if (file_stream_ && file_stream_->is_open()) {
        file_stream_->flush();
    }
    std::cout.flush();
    std::cerr.flush();
}

std::string Logger::getLogLevelString() {
    return levelToString(current_level_);
}