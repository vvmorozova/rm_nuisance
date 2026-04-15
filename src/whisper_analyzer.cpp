#include "whisper_analyzer.h"
#include "utils.h"
#include <stdexcept>

whisper_analyzer::whisper_analyzer(const std::string& model_path) {
    utils::log("whisper: loading model '" + model_path + "'");
    whisper_context_params cparams = whisper_context_default_params();
    ctx_ = whisper_init_from_file_with_params(model_path.c_str(), cparams);
    if (!ctx_) {
        throw std::runtime_error("whisper: cannot load model: " + model_path);
    }
}

std::vector<segment> whisper_analyzer::analyze(const std::vector<float>& pcm) {
    utils::log("whisper: transcribing " + std::to_string(pcm.size()) + " samples …");

    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.language            = "auto";
    params.translate           = false;
    params.token_timestamps    = true;
    params.split_on_word       = true;
    params.max_len             = 1;   // word-level segments
    params.print_progress      = false;
    params.print_realtime      = false;
    params.print_timestamps    = false;
    params.print_special       = false;
    params.no_context          = false;

    if (whisper_full(ctx_, params, pcm.data(), static_cast<int>(pcm.size())) != 0)
        throw std::runtime_error("whisper: whisper_full() failed");

    const int n_seg = whisper_full_n_segments(ctx_);
    std::vector<segment> out;
    out.reserve(n_seg);

    for (int i = 0; i < n_seg; ++i) {
        segment seg;
        seg.start_s = static_cast<float>(whisper_full_get_segment_t0(ctx_, i)) / 100.0f;
        seg.end_s   = static_cast<float>(whisper_full_get_segment_t1(ctx_, i)) / 100.0f;
        seg.text    = whisper_full_get_segment_text(ctx_, i);
        // trim leading space
        if (!seg.text.empty() && seg.text[0] == ' ')
            seg.text = seg.text.substr(1);
        out.push_back(seg);
    }

    utils::log("whisper: got " + std::to_string(n_seg) + " segments");
    return out;
}