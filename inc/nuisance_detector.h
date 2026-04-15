#pragma once

#include "storage.h"
#include <vector>
#include <regex>

class nuisance_detector {
public:
    // returns list of time ranges to cut
    std::vector<cut_range> detect(const std::vector<segment>& segs, size_t total_samples);
 
private:
    // lexical garbage patterns
 
    // fillers, hesitations, false starts
    static const std::vector<std::regex>& garbage_patterns();
 
    static bool is_garbage_text(const std::string& text);
 
    // utils
    static cut_range padded(float s, float e);
    static std::string ts(float s);
 
    static std::string trim(const std::string& s);
    static std::vector<cut_range> merge(std::vector<cut_range> ranges);
};