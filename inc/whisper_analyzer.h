#pragma once

#include "whisper.h"
#include "storage.h"
#include <vector>

class whisper_analyzer {
    whisper_context* ctx_ = nullptr;
public:
    explicit whisper_analyzer(const std::string& model_path);
 
    ~whisper_analyzer() {
        if (ctx_) whisper_free(ctx_);
    }

    // returns all transcribed segments with timestamps.
    std::vector<segment> analyze(const std::vector<float>& pcm);
};