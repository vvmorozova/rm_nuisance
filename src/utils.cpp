#include "utils.h"

#include <iostream>
#include <chrono>
#include <iomanip>

void utils::log(const std::string& msg) {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::cerr << "[" << std::put_time(std::localtime(&t), "%H:%M:%S") << "] " << msg << "\n";
}

void utils::usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [options] <input> <output>\n"
        << "\n"
        << "Options:\n"
        << "  -m <model>   Path to whisper.cpp ggml model file\n"
        << "               (default: models/ggml-base.bin)\n"
        << "\n"
        << "Examples:\n"
        << "  " << prog << " interview.mp3 interview_clean.mp3\n"
        << "  " << prog << " -m ggml-large-v3.bin lecture.wav lecture_clean.wav\n";
}