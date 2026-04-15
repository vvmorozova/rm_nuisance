#pragma once
#include "config.h"
#include <stdexcept>
#include <string>

class cmd_args_parcer {
public:
    // throw std::invalid_argument when error in args
    static config parse(int argc, char* argv[]);

private:
    static nuisance_type parse_nuisance_type(const std::string& s);
    static void validate_input_files_exist(const config& cfg);
};