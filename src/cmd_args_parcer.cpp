#include "cmd_args_parcer.h"
#include <stdexcept>
#include <string>
#include <unordered_map>

static const std::unordered_map<std::string, nuisance_type> kNuisanceMap = {
    {"filler", nuisance_type::filler},
    {"noise",  nuisance_type::noise},
    {"clicks", nuisance_type::clicks},
    {"pauses", nuisance_type::pauses},
    {"breath", nuisance_type::breath}
};

nuisance_type cmd_args_parcer::parse_nuisance_type(const std::string& s) {
    auto it = kNuisanceMap.find(s);
    if (it == kNuisanceMap.end())
        throw std::invalid_argument("Unknown nuisance type: " + s);
    return it->second;
}

config cmd_args_parcer::parse(int argc, char* argv[]) {
    if (argc < 2)
        throw std::invalid_argument("usage: rm_nuisance [options] input.wav");

    config cfg;
    // get model from config
    // "models/ggml-base.bin"
    // config by def is ~/.config/rm_nuisance/config

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // no val flag
        if (arg == "--verbose" || arg == "-v") {
            cfg.verbose = true;
            continue;
        }

        if (arg == "--pack") {
            cfg.pack_mode = true;
            // other args are input files
            while (++i < argc)
                cfg.input_files.push_back(argv[i]);
            break;
        }

        // options with value
        auto require_next = [&](const std::string& opt) -> std::string {
            if (i + 1 >= argc)
                throw std::invalid_argument(opt + " requires an argument");
            return argv[++i];
        };

        if (arg == "-i" || arg == "--input") {
            cfg.input_files.push_back(require_next(arg));
            continue;
        }

        if (arg == "-o" || arg == "--output") {
            cfg.output_file = require_next(arg);
            continue;
        }

        if (arg == "--config") {
            cfg.config_file = require_next(arg);
            continue;
        }

        if (arg == "--disable-type") {
            cfg.disabled_types.push_back(
                parse_nuisance_type(require_next(arg))
            );
            continue;
        }

        if (arg == "-m") {
            cfg.model_path = require_next(arg);
            continue;
        }

        if (arg == "--help" || arg == "-h") {
            cfg.call_help = true;
            continue;
        }

        // position arg (FR-4.1.1)
        if (arg[0] != '-') {
            cfg.input_files.push_back(arg);
            continue;
        }

        throw std::invalid_argument("Unknown option: " + arg);
    }

    if (cfg.input_files.empty())
        throw std::invalid_argument("No input files specified");

    return cfg;
}