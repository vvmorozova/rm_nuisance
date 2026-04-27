#include "nuisance_detector.h"
#include "utils.h"
#include "storage.h"
#include <algorithm>
#include <cmath>
#include "kiss_fft.h"

extern "C" {
    #include "kiss_fft.h"
}

// silence_threshold used for pcm-based silence detection
static constexpr float SILENCE_THRESHOLD = 0.01f;

std::vector<cut_range> nuisance_detector::detect(
    const std::vector<segment>& segs,
    size_t total_samples,
    const std::vector<float>& pcm)
{
    // total_samples kept in signature for api consistency, pcm.size() used instead
    (void)total_samples;

    utils::log("detector: analysing " + std::to_string(segs.size()) + " segments …");

    // local struct — use explicit constructor to stay compatible with c++14
    struct detected_nuisance {
        std::string type_name;
        cut_range   range;
        std::string detail;
        detected_nuisance(std::string tn, cut_range r, std::string d)
            : type_name(std::move(tn)), range(r), detail(std::move(d)) {}
    };

    std::vector<cut_range>         cuts;
    std::vector<detected_nuisance> details;

    // log all segments for debugging
    for (size_t i = 0; i < segs.size(); ++i) {
        utils::log("  seg[" + std::to_string(i) + "] " +
            ts(segs[i].start_s) + " – " + ts(segs[i].end_s) +
            " '" + segs[i].text + "'");
    }

    // detect garbage text segments (fillers, noise markers, stuttering, etc.)
    for (size_t i = 0; i < segs.size(); ++i) {
        const segment& seg = segs[i];
        std::string type_name;
        if (is_garbage_text(seg.text, type_name)) {
            cut_range range = padded(seg.start_s, seg.end_s);
            cuts.push_back(range);
            details.emplace_back(type_name, range, seg.text);
        }
    }

    // detect long pauses based on segment timing and PCM silence
    if (!is_type_disabled(nuisance_type::pauses)) {
        auto segment_silence = detect_silence_from_segments(segs, total_samples, MIN_SILENCE_S, 0.15f);
        for (const auto& r : segment_silence)
            details.emplace_back(nuisance_type_name(nuisance_type::pauses), r, "segment pause");
        cuts.insert(cuts.end(), segment_silence.begin(), segment_silence.end());

        auto pcm_silence = detect_silence_from_pcm(pcm, SILENCE_THRESHOLD, MIN_SILENCE_S, 0.15f);
        for (const auto& r : pcm_silence)
            details.emplace_back(nuisance_type_name(nuisance_type::pauses), r, "pcm silence");
        cuts.insert(cuts.end(), pcm_silence.begin(), pcm_silence.end());
    }

    // detect clicks (sharp amplitude spikes)
    if (!is_type_disabled(nuisance_type::clicks)) {
        auto click_cuts = detect_clicks_from_pcm(pcm);
        for (const auto& r : click_cuts)
            details.emplace_back(nuisance_type_name(nuisance_type::clicks), r, "pcm click");
        cuts.insert(cuts.end(), click_cuts.begin(), click_cuts.end());
    }

    // detect mechanical noise / rustle (high-frequency energy bursts)
    if (!is_type_disabled(nuisance_type::noise)) {
        // ratio_threshold=0.4f:
        auto rustle_cuts = detect_rustle_from_pcm(pcm, 0.3f, 0.3f);
        for (const auto& r : rustle_cuts)
            details.emplace_back(nuisance_type_name(nuisance_type::noise), r, "pcm rustle");
        cuts.insert(cuts.end(), rustle_cuts.begin(), rustle_cuts.end());
    }

    cuts = merge(cuts);
    utils::log("detector: " + std::to_string(cuts.size()) + " cut range(s) identified");

    if (!details.empty()) {
        utils::log("detector: nuisance details:");
        for (const auto& item : details) {
            std::string line = "  [" + item.type_name + "] " +
                ts(item.range.start_s) + " - " + ts(item.range.end_s);
            if (!item.detail.empty())
                line += " (" + item.detail + ")";
            utils::log(line);
        }
    }

    return cuts;
}

// pcm silence detection 

std::vector<cut_range> nuisance_detector::detect_silence_from_pcm(
    const std::vector<float>& pcm, float silence_threshold,
    float min_silence_s, float keep_pad_s)
{
    // 20 ms analysis window
    const int window_samples      = static_cast<int>(0.02f * SAMPLE_RATE);
    const int min_silence_samples = static_cast<int>(min_silence_s * SAMPLE_RATE);

    // compute rms energy per window and mark silent windows
    std::vector<bool> is_silent;
    is_silent.reserve(pcm.size() / window_samples + 1);

    for (size_t i = 0; i < pcm.size(); i += window_samples) {
        size_t end = std::min(i + static_cast<size_t>(window_samples), pcm.size());
        float rms = 0.0f;
        for (size_t j = i; j < end; ++j)
            rms += pcm[j] * pcm[j];
        rms = std::sqrt(rms / static_cast<float>(end - i));
        is_silent.push_back(rms < silence_threshold);
    }

    // find contiguous silent regions longer than min_silence_s
    std::vector<cut_range> cuts;
    int silence_start = -1;

    auto try_add_cut = [&](int start_w, int end_w) {
        int len_samples = (end_w - start_w) * window_samples;
        if (len_samples < min_silence_samples)
            return;
        float s = static_cast<float>(start_w * window_samples) / SAMPLE_RATE;
        float e = static_cast<float>(end_w   * window_samples) / SAMPLE_RATE;
        // keep a small natural pad at each edge so the cut isn't too abrupt
        float cut_s = s + keep_pad_s;
        float cut_e = e - keep_pad_s;
        if (cut_e > cut_s + 0.1f) {
            cuts.push_back({cut_s, cut_e});
            utils::log("detector: [pcm silence] " + ts(cut_s) + " – " + ts(cut_e) +
                " (" + std::to_string(cut_e - cut_s) + " s)");
        }
    };

    for (size_t w = 0; w < is_silent.size(); ++w) {
        if (is_silent[w] && silence_start < 0) {
            // start of a silent region
            silence_start = static_cast<int>(w);
        } else if (!is_silent[w] && silence_start >= 0) {
            // end of a silent region
            try_add_cut(silence_start, static_cast<int>(w));
            silence_start = -1;
        }
    }

    // handle trailing silence
    if (silence_start >= 0)
        try_add_cut(silence_start, static_cast<int>(is_silent.size()));

    return cuts;
}

std::vector<cut_range> nuisance_detector::detect_silence_from_segments(
    const std::vector<segment>& segs,
    size_t total_samples,
    float min_silence_s,
    float keep_pad_s)
{
    std::vector<cut_range> cuts;
    if (segs.empty())
        return cuts;

    std::vector<segment> sorted_segs = segs;
    std::sort(sorted_segs.begin(), sorted_segs.end(),
        [](const segment& a, const segment& b) {
            return a.start_s < b.start_s;
        });

    const float total_duration = static_cast<float>(total_samples) / SAMPLE_RATE;

    auto try_add_cut = [&](float start_s, float end_s) {
        float cut_s = std::max(0.0f, start_s + keep_pad_s);
        float cut_e = std::min(total_duration, end_s - keep_pad_s);
        if (cut_e > cut_s + 0.1f) {
            cuts.push_back({cut_s, cut_e});
            utils::log("detector: [segment pause] " + ts(cut_s) + " – " + ts(cut_e) +
                " (" + std::to_string(cut_e - cut_s) + " s)");
        }
    };

    const segment& first_seg = sorted_segs.front();
    if (first_seg.start_s > min_silence_s) {
        try_add_cut(0.0f, first_seg.start_s);
    }

    for (size_t i = 1; i < sorted_segs.size(); ++i) {
        const segment& prev = sorted_segs[i - 1];
        const segment& cur = sorted_segs[i];
        float gap = cur.start_s - prev.end_s;
        if (gap > min_silence_s) {
            try_add_cut(prev.end_s, cur.start_s);
        }
    }

    const segment& last_seg = sorted_segs.back();
    if (total_duration - last_seg.end_s > min_silence_s) {
        try_add_cut(last_seg.end_s, total_duration);
    }

    return cuts;
}

//  garbage text classification 

bool nuisance_detector::is_garbage_text(const std::string& text, std::string& out_type_name)
{
    if (text.empty()) {
        out_type_name = "garbage";
        return true;
    }

    std::string t = trim_punct(text);

    // empty or whitespace-only after stripping punctuation
    if (t.empty() || t.find_first_not_of(" \t") == std::string::npos) {
        out_type_name = "garbage";
        return true;
    }

    for (const auto& entry : pats_) {
        if (std::regex_match(t, entry.pattern)) {
            out_type_name = nuisance_type_name(entry.type);
            return true;
        }
    }
    utils::log("  [not matched] '" + t + "' (original: '" + text + "')");

    return false;
}

// pattern building 

void nuisance_detector::build_patterns()
{
    pats_.clear();

    if (!is_type_disabled(nuisance_type::filler)) {
        utils::log("building filler patterns");
        // russian hesitation sounds: э-э-э, м-м-м, а-а-а, etc.
        pats_.push_back({nuisance_type::filler, std::regex(
            u8"^[эЭмМаАнНоО][\\-–—]?[эЭмМаАнНоО][\\-–—]?[эЭмМаАнНоО]?$",
            std::regex::icase | std::regex::ECMAScript)});
        // russian filler words
        pats_.push_back({nuisance_type::filler, std::regex(
            u8"^(хм|хмм|гм|эх|ой|ах|ну|вот|так|это|блин)$",
            std::regex::icase | std::regex::ECMAScript)});
        // english filler words
        pats_.push_back({nuisance_type::filler, std::regex(
            "^(um|uh|hmm|hm|ah|oh|er|err|ugh|phew|ew)$",
            std::regex::icase | std::regex::ECMAScript)});
    }

    if (!is_type_disabled(nuisance_type::clicks)) {
        // stuttering: repeated word ("я я", "он он он")
        pats_.push_back({nuisance_type::clicks, std::regex(
            u8"^(\\w+)\\s+\\1(\\s+\\1)*$",
            std::regex::icase | std::regex::ECMAScript)});
        // click/pop markers emitted by whisper
        pats_.push_back({nuisance_type::clicks, std::regex(
            "^\\[(click|pop|snap|tap|knock)\\]$",
            std::regex::icase | std::regex::ECMAScript)});
    }

    if (!is_type_disabled(nuisance_type::noise)) {
        utils::log("building noise patterns");
        // bracketed noise tags whisper may emit
        pats_.push_back({nuisance_type::noise, std::regex(
            "^\\[(breath|inhale|exhale|cough|smack|click|noise|music"
            "|дыхание|вдох|выдох|кашель|щелчок|шум|музыка)\\]$",
            std::regex::icase | std::regex::ECMAScript)});
        // same words without brackets
        pats_.push_back({nuisance_type::noise, std::regex(
            "^(breath|inhale|exhale|cough|smack|click|noise|music"
            "|дыхание|вдох|выдох|кашель|щелчок|шум|музыка)$",
            std::regex::icase | std::regex::ECMAScript)});
    }

    if (!is_type_disabled(nuisance_type::breath)) {
        utils::log("building breath patterns");
        // short breath/exhale syllables
        pats_.push_back({nuisance_type::breath, std::regex(
            u8"^(ah|uh|huh|hm|eh|oh|ha|he|hi|ho|hu)$",
            std::regex::icase | std::regex::ECMAScript)});
        // tokens with no letters at all (pure punctuation/symbols)
        pats_.push_back({nuisance_type::breath, std::regex(
            u8"^[^а-яёА-ЯЁa-zA-Z]*$",
            std::regex::ECMAScript)});
    }
}

//  helpers 

cut_range nuisance_detector::padded(float s, float e)
{
    return { std::max(0.0f, s - GARBAGE_PAD_S), e + GARBAGE_PAD_S };
}

std::string nuisance_detector::ts(float s)
{
    int m = static_cast<int>(s) / 60;
    float sec = s - m * 60;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d:%05.2f", m, sec);
    return buf;
}

std::string nuisance_detector::trim_punct(const std::string& s)
{
    // strip leading/trailing whitespace, punctuation and non-breaking spaces
    const std::string chars = " \t\r\n.!?,;:\xc2\xa0";
    size_t start = s.find_first_not_of(chars);
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(chars);
    return s.substr(start, end - start + 1);
}

std::string nuisance_detector::nuisance_type_name(nuisance_type type)
{
    switch (type) {
        case nuisance_type::filler: return "filler";
        case nuisance_type::noise:  return "noise";
        case nuisance_type::clicks: return "clicks";
        case nuisance_type::pauses: return "pauses";
        case nuisance_type::breath: return "breath";
    }
    return "unknown";
}

std::vector<cut_range> nuisance_detector::merge(std::vector<cut_range> ranges)
{
    if (ranges.empty()) return ranges;

    std::sort(ranges.begin(), ranges.end(),
        [](const cut_range& a, const cut_range& b){ return a.start_s < b.start_s; });

    std::vector<cut_range> out;
    out.push_back(ranges[0]);

    for (size_t i = 1; i < ranges.size(); ++i) {
        // merge overlapping or nearly-adjacent ranges (within 50 ms)
        if (ranges[i].start_s <= out.back().end_s + 0.05f)
            out.back().end_s = std::max(out.back().end_s, ranges[i].end_s);
        else
            out.push_back(ranges[i]);
    }

    return out;
}

bool nuisance_detector::is_type_disabled(nuisance_type type)
{
    return std::find(disable_types.begin(), disable_types.end(), type) != disable_types.end();
}

std::vector<cut_range> nuisance_detector::detect_clicks_from_pcm(
    const std::vector<float>& pcm,
    float peak_threshold,
    float context_s)
{
    const int context_samples = static_cast<int>(context_s * SAMPLE_RATE);
    std::vector<cut_range> cuts;

    for (size_t i = 1; i + 1 < pcm.size(); ++i) {
        float val = std::abs(pcm[i]);
        if (val < peak_threshold) continue;

        // peak is higher that neighborn, it excludes loud sound
        float prev_avg = std::abs(pcm[i-1]);
        float next_avg = std::abs(pcm[i+1]);
        float local_ctx = (prev_avg + next_avg) / 2.0f;

        if (val > local_ctx * 3.0f) {
            float s = std::max(0.0f,
                static_cast<float>(static_cast<int>(i) - context_samples) / SAMPLE_RATE);
            float e = std::min(static_cast<float>(pcm.size()) / SAMPLE_RATE,
                static_cast<float>(static_cast<int>(i) + context_samples) / SAMPLE_RATE);
            utils::log("detector: [click] @ " + ts(s) + " – " + ts(e) +
                " peak=" + std::to_string(val));
            cuts.push_back({s, e});
            // let context_samples be firts to exclude doubles
            i += context_samples;
        }
    }

    return merge(cuts);
}

std::vector<cut_range> nuisance_detector::detect_rustle_from_pcm(
    const std::vector<float>& pcm,
    float ratio_threshold,
    float min_duration_s)
{
    const int window_samples    = 512;   // must be power of 2 for kiss_fft
    const int min_noisy_windows = static_cast<int>(min_duration_s * SAMPLE_RATE / window_samples);

    // indexes freq bins for 16000 Gz and window 512 samples
    // bin = freq * window_samples / sample_rate
    // speech:      300-3000 Gz → bins 10-96
    // rustle: 4000-8000 Gz → bins 128-256
    const int lf_bin_start = 10,  lf_bin_end = 96;
    const int hf_bin_start = 128, hf_bin_end = 256;

    kiss_fft_cfg cfg = kiss_fft_alloc(window_samples, 0, nullptr, nullptr);
    if (!cfg)
        throw std::runtime_error("detector: kiss_fft_alloc failed");

    std::vector<kiss_fft_cpx> in_buf(window_samples);
    std::vector<kiss_fft_cpx> out_buf(window_samples);
    std::vector<bool> is_noisy;

    for (size_t i = 0; i + window_samples < pcm.size(); i += window_samples) {
        // fill input buffer (kiss_fft works with complex numbers)
        for (int j = 0; j < window_samples; ++j) {
            // apply Hann window to reduce spectral leakage
            float hann = 0.5f * (1.0f - std::cos(2.0f * M_PI * j / (window_samples - 1)));
            in_buf[j].r = pcm[i + j] * hann;
            in_buf[j].i = 0.0f;
        }

        kiss_fft(cfg, in_buf.data(), out_buf.data());

        // calculate energy in low-frequency and high-frequency bands
        float lf_energy = 0.0f;
        for (int b = lf_bin_start; b <= lf_bin_end; ++b)
            lf_energy += out_buf[b].r * out_buf[b].r + out_buf[b].i * out_buf[b].i;

        float hf_energy = 0.0f;
        for (int b = hf_bin_start; b <= hf_bin_end; ++b)
            hf_energy += out_buf[b].r * out_buf[b].r + out_buf[b].i * out_buf[b].i;

        float total = lf_energy + hf_energy;
        float ratio = (total > 1e-6f) ? (hf_energy / total) : 0.0f;

        is_noisy.push_back(ratio > ratio_threshold);
    }

    kiss_fft_free(cfg);

    // collect continuous noisy segments
    std::vector<cut_range> cuts;
    int noisy_start = -1, noisy_count = 0;

    for (size_t w = 0; w < is_noisy.size(); ++w) {
        if (is_noisy[w]) {
            if (noisy_start < 0) { noisy_start = static_cast<int>(w); noisy_count = 0; }
            ++noisy_count;
        } else {
            if (noisy_start >= 0 && noisy_count >= min_noisy_windows) {
                float s = static_cast<float>(noisy_start * window_samples) / SAMPLE_RATE;
                float e = static_cast<float>(w * window_samples) / SAMPLE_RATE;
                utils::log("detector: [rustle] " + ts(s) + " – " + ts(e));
                cuts.push_back({s, e});
            }
            noisy_start = -1; noisy_count = 0;
        }
    }
    // tail noisy segment
    if (noisy_start >= 0 && noisy_count >= min_noisy_windows) {
        float s = static_cast<float>(noisy_start * window_samples) / SAMPLE_RATE;
        float e = static_cast<float>(pcm.size()) / SAMPLE_RATE;
        cuts.push_back({s, e});
    }

    return merge(cuts);
}