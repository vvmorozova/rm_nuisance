#include "utils.h"
#include "orchestrator.h"

void orchestrator::run(const std::string& input_path, const std::string& output_path) {
    utils::log("rm nuisance pipeline started");
    utils::log("input  : " + input_path);
    utils::log("output : " + output_path);

    // decode
    auto pcm = decoder_.decode(input_path);

    // whisper transcription & timestamps
    auto segments = whisper_.analyze(pcm);

    // detect garbage ranges
    auto cuts = detector_.detect(segments, pcm.size());

    if (cuts.empty()) {
        utils::log("No garbage found – writing output unchanged");
        encoder_.encode(pcm, output_path);
        return;
    }

    // erase segments & fade
    auto clean_pcm = eraser_.erase(pcm, cuts);

    // encode output
    encoder_.encode(clean_pcm, output_path);

    utils::log("Pipeline finished");
}
