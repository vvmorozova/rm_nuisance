#include "cmd_args_parcer.h"
#include "orchestrator.h"
#include "utils.h"
#include <iostream>

int main(int argc, char **argv)
{
    try {
        auto new_config = cmd_args_parcer::parse(argc, argv);
        orchestrator pipeline(new_config);
        pipeline.run();
    } catch (const std::exception& ex) {
        std::cerr << "[ERROR] " << ex.what() << "\n";
        return 2;
    }
 
    return 0;
}