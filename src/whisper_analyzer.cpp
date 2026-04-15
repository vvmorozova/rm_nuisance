#include "whisper_analyzer.h"
#include "utils.h"
#include <stdexcept>

whisper_analyzer::whisper_analyzer(const std::string& model_path) {
    utils::log("Whisper: loading model '" + model_path + "'");
    whisper_context_params cparams = whisper_context_default_params();
    ctx_ = whisper_init_from_file_with_params(model_path.c_str(), cparams);
    if (!ctx_) {
        throw std::runtime_error("Whisper: cannot load model: " + model_path);
    }
}