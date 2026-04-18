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
    std::cerr << "usage: " << prog << " [options] <input> <output>\n"
        << "\n"
        << "options:\n"
        << "  -m <model>   path to whisper.cpp ggml model file\n"
        << "               (default: models/ggml-base.bin)\n"
        << "\n"
        << "  -i <input>   input files list\n"
        << "               (with no flag only one file can be choosen)\n"
        << "\n"
        << "  -o <output>  output files list"
        << "               (with no flag output files will be named like this: <input filename>-output.<input file format>)\n"
        << "\n"
        << "  --disable-type [filler|noise|clicks|pauses|breath] disable processing types"
        << "               (default: none are disabled)\n"
        << "\n"
        << "  --verbose    verbose processing output"
        << "\n"
        << "examples:\n"
        << "  " << prog << " interview.mp3 interview_clean.mp3\n"
        << "  " << prog << " -m ggml-large-v3.bin lecture.wav lecture_clean.wav\n";
}
