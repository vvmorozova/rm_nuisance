#include "cmd_args_parcer.h"
#include "orchestrator.h"
#include "utils.h"
#include <iostream>

int main(int argc, char **argv)
{
    auto newConfig = cmd_args_parcer::parse(argc, argv);
    std::string model_path = "models/ggml-base.bin";
    std::string input_path = newConfig.input_files.size() == 0 ? "" : newConfig.input_files[0];
    std::string output_path = newConfig.output_file;
 
    if (input_path.empty() || output_path.empty()) {
        utils::usage(argv[0]);
        return 1;
    }
 
    try {
        orchestrator pipeline(model_path);
        pipeline.run(input_path, output_path);
    } catch (const std::exception& ex) {
        std::cerr << "[ERROR] " << ex.what() << "\n";
        return 2;
    }
 
    return 0;
}