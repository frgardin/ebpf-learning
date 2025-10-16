#pragma once
#include <iostream>
#include <string>

class Logger
{
public:
    static void info(const std::string &msg) { std::cout << "[INFO] " << msg << std::endl; }
    static void debug(const std::string &msg) { std::cout << "[DEBUG] " << msg << std::endl; }
    static void error(const std::string &msg) { std::cerr << "[ERROR] " << msg << std::endl; }
};
