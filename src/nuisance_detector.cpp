#include "nuisance_detector.h"
#include "utils.h"
#include "storage.h"
#include <cmath>

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

    // detect silence directly from pcm — whisper often hallucinates text over silent regions
    if (!is_type_disabled(nuisance_type::pauses)) {
        auto silence_cuts = detect_silence_from_pcm(pcm, SILENCE_THRESHOLD, MIN_SILENCE_S, 0.15f);
        for (const auto& r : silence_cuts)
            details.emplace_back(nuisance_type_name(nuisance_type::pauses), r, "pcm silence");
        cuts.insert(cuts.end(), silence_cuts.begin(), silence_cuts.end());
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

// ─── garbage text classification ─────────────────────────────────────────────

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

    return false;
}

// ─── pattern building ─────────────────────────────────────────────────────────

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

// ─── helpers ──────────────────────────────────────────────────────────────────

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