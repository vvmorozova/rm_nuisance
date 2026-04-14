#pragma once
#include <string>
#include <vector>

enum class nuisance_type {
    Filler,
    Noise,
    Clicks,
    Pauses,
    Breath
};

struct config {
    std::vector<std::string> input_files;
    std::string              output_file;
    std::string              config_file;
    std::vector<nuisance_type> disabled_types;
    bool                     pack_mode = false;
    bool                     verbose   = false;
};