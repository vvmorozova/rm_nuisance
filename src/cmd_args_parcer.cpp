#include "cmd_args_parcer.h"
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <iostream>
#include <filesystem>

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
        throw std::invalid_argument("execute rm_nuisance -h to see help");

    config cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // options with multiple values
        auto collect_values = [&](std::vector<std::string>& target) {
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                target.push_back(argv[++i]);
            }
        };

        if (arg == "-i" || arg == "--input") {
            collect_values(cfg.input_files);
            continue;
        }

        if (arg == "-o" || arg == "--output") {
            collect_values(cfg.output_files);
            continue;
        }

        // options with single value
        auto require_next = [&](const std::string& opt) -> std::string {
            if (i + 1 >= argc || argv[i + 1][0] == '-')
                throw std::invalid_argument(opt + " requires an argument");
            return argv[++i];
        };

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

        // positional args
        if (arg[0] != '-') {
            cfg.input_files.push_back(arg);
            continue;
        }

        throw std::invalid_argument("unknown option: " + arg);
    }

    validate_input_files_exist(cfg);

    int inSize = cfg.input_files.size();
    int outSize = cfg.output_files.size();

    if (inSize == 0)
        throw std::invalid_argument("no input files specified");

    if (inSize > 1 && outSize > 0 && outSize != inSize) {
        std::cout << inSize << " " << outSize << std::endl;
        throw std::invalid_argument("input and output files numbers don't match");
    }

    return cfg;
}

void cmd_args_parcer::validate_input_files_exist(const config& cfg) {
    std::vector<std::string> missing;

    for (const auto& file : cfg.input_files) {
        if (!std::filesystem::exists(file)) {
            missing.push_back(file);
        }
    }

    if (!missing.empty()) {
        std::string msg = "input files do not exist: ";
        for (size_t i = 0; i < missing.size(); ++i) {
            msg += missing[i];
            if (i + 1 < missing.size())
                msg += ", ";
        }
        throw std::runtime_error(msg);
    }
}
