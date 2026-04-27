#include "cmd_args_parcer.h"
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <iostream>
#include <filesystem>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

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

    po::options_description desc("allowed options");
    desc.add_options()
        ("help,h",                                                        "show help")
        ("input,i",    po::value<std::vector<std::string>>()->multitoken(), "input files")
        ("output,o",   po::value<std::vector<std::string>>()->multitoken(), "output files")
        ("disable-type", po::value<std::vector<std::string>>()->multitoken(), "disable nuisance type")
        ("model,m",    po::value<std::string>(),                           "model path");

    // collect positional args into --input
    po::positional_options_description pos;
    pos.add("input", -1);

    po::variables_map vm;
    po::store(
        po::command_line_parser(argc, argv)
            .options(desc)
            .positional(pos)
            .run(),
        vm
    );
    po::notify(vm);

    if (vm.count("help"))
        cfg.call_help = true;

    if (vm.count("input"))
        cfg.input_files = vm["input"].as<std::vector<std::string>>();

    if (vm.count("output"))
        cfg.output_files = vm["output"].as<std::vector<std::string>>();

    if (vm.count("disable-type")) {
        for (const auto& s : vm["disable-type"].as<std::vector<std::string>>())
            cfg.disabled_types.push_back(parse_nuisance_type(s));
    }

    if (vm.count("model"))
        cfg.model_path = vm["model"].as<std::string>();

    validate_input_files_exist(cfg);

    int inSize  = cfg.input_files.size();
    int outSize = cfg.output_files.size();

    if (inSize == 0 && !cfg.call_help)
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
        if (!std::filesystem::exists(file))
            missing.push_back(file);
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