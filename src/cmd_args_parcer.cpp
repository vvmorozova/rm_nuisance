#include "cmd_args_parcer.h"
#include <stdexcept>
#include <string>
#include <unordered_map>

static const std::unordered_map<std::string, nuisance_type> kNuisanceMap = {
    {"filler", nuisance_type::Filler},
    {"noise",  nuisance_type::Noise},
    {"clicks", nuisance_type::Clicks},
    {"pauses", nuisance_type::Pauses},
    {"breath", nuisance_type::Breath}
};

nuisance_type cmd_args_parcer::parse_nuisance_type(const std::string& s) {
    auto it = kNuisanceMap.find(s);
    if (it == kNuisanceMap.end())
        throw std::invalid_argument("Unknown nuisance type: " + s);
    return it->second;
}

config ArgsParser::parse(int argc, char* argv[]) {
    if (argc < 2)
        throw std::invalid_argument("Usage: rm_nuisance [options] input.wav");

    config cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // --- Флаги без значения ---
        if (arg == "--verbose" || arg == "-v") {
            cfg.verbose = true;
            continue;
        }

        if (arg == "--pack") {
            cfg.pack_mode = true;
            // все оставшиеся аргументы — входные файлы
            while (++i < argc)
                cfg.input_files.push_back(argv[i]);
            break;
        }

        // --- Опции со значением ---
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

        // --- Позиционный аргумент (FR-4.1.1) ---
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