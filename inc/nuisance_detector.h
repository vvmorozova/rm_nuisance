#pragma once

#include "storage.h"
#include "config.h"
#include <vector>
#include <regex>

class nuisance_detector {
    const std::vector<nuisance_type> &disable_types;
    std::vector<std::regex> pats_;

public:
    // returns list of time ranges to cut
    std::vector<cut_range> detect(const std::vector<segment>& segs, size_t total_samples);

    explicit nuisance_detector(const std::vector<nuisance_type> & cfg_disable_types) : disable_types(cfg_disable_types) {};
 
    ~nuisance_detector() {};

    void add_pattern(const std::string& pattern,
                    std::regex_constants::syntax_option_type flags =
                         std::regex::icase | std::regex::ECMAScript) {
        pats_.emplace_back(pattern, flags);
    }
 
private: 
    bool is_garbage_text(const std::string& text);
 
    // utils
    cut_range padded(float s, float e);
    std::string ts(float s);
 
    std::string trim(const std::string& s);
    std::vector<cut_range> merge(std::vector<cut_range> ranges);

    bool is_type_disabled(nuisance_type);
    void build_patterns();
};