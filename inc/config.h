#pragma once

#include <string>
#include <vector>

enum class nuisance_type {
    filler,
    noise,
    clicks,
    pauses,
    breath
};

struct config {
    std::vector<std::string>    input_files;
    std::vector<std::string>    output_files;
    std::string                 config_file;
    std::string                 model_path = "models/ggml-base.bin";
    std::vector<nuisance_type>  disabled_types;
    bool                        pack_mode = false;
    bool                        call_help = false;
};