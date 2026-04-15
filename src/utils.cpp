#include "utils.h"

#include <iostream>
#include <chrono>
#include <iomanip>

void utils::log(const std::string& msg) {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::cerr << "[" << std::put_time(std::localtime(&t), "%H:%M:%S") << "] " << msg << "\n";
}