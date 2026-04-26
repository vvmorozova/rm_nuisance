#include "utils.h"
#include "orchestrator.h"

void orchestrator::run() {
    int inSize = cfg_.input_files.size();
    int outSize = cfg_.output_files.size();

    for (int i = 0; i < inSize; i++) {
        if (inSize > 1) {
            std::string msg = "processing " + std::to_string(i + 1) + " of " + std::to_string(inSize) + " files";
            utils::log(msg);
        }

        utils::log("rm nuisance pipeline started");
        std::string input_path = cfg_.input_files[i];
        std::string output_path = outSize > 0 ? cfg_.output_files[i] : generate_output_name(input_path);
        utils::log("input  : " + input_path);
        utils::log("output : " + output_path);
    
        // decode
        auto pcm = decoder_.decode(input_path);
    
        // whisper transcription & timestamps
        auto segments = whisper_.analyze(pcm);
    
        // detect nuisance ranges
        auto cuts = detector_.detect(segments, pcm.size());
    
        if (cuts.empty()) {
            utils::log("no nuisance found – writing output unchanged");
            encoder_.encode(pcm, output_path);
            continue;
        }
    
        // erase segments & fade
        auto clean_pcm = eraser_.erase(pcm, cuts);
    
        // encode output
        encoder_.encode(clean_pcm, output_path);
    
        utils::log("pipeline finished");
    }
}

std::string orchestrator::generate_output_name(std::string input)
{
    std::size_t pos = input.rfind('.');
    if (pos == std::string::npos) {
        return input + "-output";
    }
    return input.substr(0, pos) + "-output" + input.substr(pos);
}
