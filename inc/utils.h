#pragma once

#include <string>

// whisper requires 16 kHz mono float32 !! todo: get from config(?)
static constexpr int   SAMPLE_RATE      = 16000; 
// 25 ms fade-in / fade-out  !! todo: get from config
static constexpr float FADE_DURATION_S  = 0.025f;
// pause longer than this → cut !! todo: get from config
static constexpr float MIN_SILENCE_S    = 2.0f;
// extra padding around each bad segment !! todo: get from config
static constexpr float GARBAGE_PAD_S   = 0.05f;

namespace utils {
    void log(const std::string& msg);

    void usage(const char* prog);
}

