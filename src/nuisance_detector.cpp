#include "nuisance_detector.h"
#include "utils.h"
#include "storage.h"
/*
enum class nuisance_type {
    filler,
    noise,
    clicks,
    pauses,
    breath
};
*/

std::vector<cut_range> nuisance_detector::detect(const std::vector<segment>& segs, size_t total_samples)
{
    utils::log("detector: analysing " + std::to_string(segs.size()) + " segments …");
 
    std::vector<cut_range> cuts;

    for (size_t i = 0; i < segs.size(); ++i) {
        const segment& seg = segs[i];

        if (is_garbage_text(seg.text)) {
            utils::log("  [garbage text]  " + seg.text +
                " @ " + ts(seg.start_s) + " – " + ts(seg.end_s));
            cuts.push_back(padded(seg.start_s, seg.end_s));
        }

        // long silence between current segment end and next segment
        if (i + 1 < segs.size()) {
            float gap = segs[i+1].start_s - seg.end_s;
            if (gap >= MIN_SILENCE_S) {
                utils::log("  [long pause]  " + std::to_string(gap) + "s"
                    " @ " + ts(seg.end_s) + " – " + ts(segs[i+1].start_s));
                // Keep a small natural gap (0.3 s) instead of the whole silence
                float cut_start = seg.end_s + 0.15f;
                float cut_end   = segs[i+1].start_s - 0.15f;
                if (cut_end > cut_start + 0.1f)
                    cuts.push_back({cut_start, cut_end});
            }
        }
    }

    // silence
    if (!is_type_disabled(nuisance_type::pauses) && !segs.empty()) {
        float first_start = segs.front().start_s;
        if (first_start > MIN_SILENCE_S)
            cuts.push_back({0.3f, first_start - 0.15f});

        float total_s = static_cast<float>(total_samples) / SAMPLE_RATE;
        float last_end = segs.back().end_s;
        if (total_s - last_end > MIN_SILENCE_S)
            cuts.push_back({last_end + 0.15f, total_s - 0.3f});
    }

    // merge overlapping / adjacent ranges
    cuts = merge(cuts);
    utils::log("detector: " + std::to_string(cuts.size()) + " cut range(s) identified");
    return cuts;
}

bool nuisance_detector::is_garbage_text(const std::string& text) {
    if (text.empty()) return true;

    std::string t = trim(text);
    if (t.empty()) return true;

    for (const auto& pat : pats_)
        if (std::regex_match(t, pat)) return true;

    return false;
}

cut_range nuisance_detector::padded(float s, float e) {
    return { std::max(0.0f, s - GARBAGE_PAD_S), e + GARBAGE_PAD_S };
}

std::string nuisance_detector::ts(float s) {
    int m = static_cast<int>(s) / 60;
    float sec = s - m * 60;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d:%05.2f", m, sec);
    return buf;
}

std::string nuisance_detector::trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::vector<cut_range> nuisance_detector::merge(std::vector<cut_range> ranges) {
    if (ranges.empty()) return ranges;
    std::sort(ranges.begin(), ranges.end(),
                [](const cut_range& a, const cut_range& b){ return a.start_s < b.start_s; });
    std::vector<cut_range> out;
    out.push_back(ranges[0]);
    for (size_t i = 1; i < ranges.size(); ++i) {
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

void nuisance_detector::build_patterns()
{
    pats_.clear();

    if (!is_type_disabled(nuisance_type::filler)) {
        pats_.emplace_back(
            u8"^[эЭмМаАнНоО][\\-–—]?[эЭмМаАнНоО][\\-–—]?[эЭмМаАнНоО]?$",
            std::regex::icase | std::regex::ECMAScript);
        pats_.emplace_back(
            u8"^(хм|хмм|гм|эх|ой|ах|ну|вот|так|это|блин)$",
            std::regex::icase | std::regex::ECMAScript);
        pats_.emplace_back(
            "^(um|uh|hmm|hm|ah|oh|er|err|ugh|phew|ew)$",
            std::regex::icase | std::regex::ECMAScript);
    }

    if (!is_type_disabled(nuisance_type::clicks)) {
        pats_.emplace_back(
            u8"^(\\w+)\\s+\\1(\\s+\\1)*$",
            std::regex::icase | std::regex::ECMAScript);
    }

    if (!is_type_disabled(nuisance_type::noise)) {
        pats_.emplace_back(
            "^\\[(breath|inhale|exhale|cough|smack|click|noise|music)\\]$",
            std::regex::icase | std::regex::ECMAScript);
    }

    if (!is_type_disabled(nuisance_type::breath)) {
        pats_.emplace_back(
            u8"^[^а-яёА-ЯЁa-zA-Z]*$",
            std::regex::ECMAScript);
    }
}
