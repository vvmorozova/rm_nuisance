#include "nuisance_detector.h"
#include "utils.h"
#include "storage.h"

std::vector<cut_range> nuisance_detector::detect(const std::vector<segment>& segs, size_t total_samples)
{
    utils::log("detector: analysing " + std::to_string(segs.size()) + " segments …");

    struct detected_nuisance {
        std::string type_name;
        cut_range range;
        std::string detail;
    };

    std::vector<cut_range> cuts;
    std::vector<detected_nuisance> details;

    for (size_t i = 0; i < segs.size(); ++i) {
        const segment& seg = segs[i];

        std::string type_name;
        if (is_garbage_text(seg.text, type_name)) {
            cut_range range = padded(seg.start_s, seg.end_s);
            cuts.push_back(range);
            details.push_back({type_name, range, seg.text});
        }

        if (!is_type_disabled(nuisance_type::pauses)) {
            if (i + 1 < segs.size()) {
                float gap = segs[i+1].start_s - seg.end_s;
                if (gap >= MIN_SILENCE_S) {
                    float cut_start = seg.end_s + 0.15f;
                    float cut_end   = segs[i+1].start_s - 0.15f;
                    if (cut_end > cut_start + 0.1f) {
                        cut_range range = {cut_start, cut_end};
                        cuts.push_back(range);
                        details.push_back({nuisance_type_name(nuisance_type::pauses), range, "silent gap"});
                    }
                }
            }
        }
    }

    if (!is_type_disabled(nuisance_type::pauses) && !segs.empty()) {
        utils::log("detector: pauses detected");
        float first_start = segs.front().start_s;
        if (first_start > MIN_SILENCE_S) {
            cut_range range = {0.3f, first_start - 0.15f};
            cuts.push_back(range);
            details.push_back({nuisance_type_name(nuisance_type::pauses), range, "leading silence"});
        }

        float total_s = static_cast<float>(total_samples) / SAMPLE_RATE;
        float last_end = segs.back().end_s;
        if (total_s - last_end > MIN_SILENCE_S) {
            cut_range range = {last_end + 0.15f, total_s - 0.3f};
            cuts.push_back(range);
            details.push_back({nuisance_type_name(nuisance_type::pauses), range, "trailing silence"});
        }
    }

    cuts = merge(cuts);
    utils::log("detector: " + std::to_string(cuts.size()) + " cut range(s) identified");

    if (!details.empty()) {
        utils::log("detector: nuisance details:");
        for (const auto& item : details) {
            std::string line = "  [" + item.type_name + "] " + ts(item.range.start_s) + " - " + ts(item.range.end_s);
            if (!item.detail.empty()) {
                line += " (" + item.detail + ")";
            }
            utils::log(line);
        }
    }

    return cuts;
}

bool nuisance_detector::is_garbage_text(const std::string& text, std::string &out_type_name) {
    if (text.empty()) {
        out_type_name = "garbage";
        return true;
    }

    std::string t = trim_punct(text);
    if (t.empty()) {
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

std::string nuisance_detector::trim_punct(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n.!?,;:");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n.!?,;:");
    return s.substr(start, end - start + 1);
}

std::string nuisance_detector::nuisance_type_name(nuisance_type type) {
    switch (type) {
        case nuisance_type::filler: return "filler";
        case nuisance_type::noise:  return "noise";
        case nuisance_type::clicks: return "clicks";
        case nuisance_type::pauses: return "pauses";
        case nuisance_type::breath: return "breath";
    }
    return "unknown";
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
        utils::log("building filler patterns");
        pats_.push_back({nuisance_type::filler, std::regex(
            u8"^[эЭмМаАнНоО][\\-–—]?[эЭмМаАнНоО][\\-–—]?[эЭмМаАнНоО]?$",
            std::regex::icase | std::regex::ECMAScript)});
        pats_.push_back({nuisance_type::filler, std::regex(
            u8"^(хм|хмм|гм|эх|ой|ах|ну|вот|так|это|блин)$",
            std::regex::icase | std::regex::ECMAScript)});
        pats_.push_back({nuisance_type::filler, std::regex(
            "^(um|uh|hmm|hm|ah|oh|er|err|ugh|phew|ew)$",
            std::regex::icase | std::regex::ECMAScript)});
    }

    if (!is_type_disabled(nuisance_type::clicks)) {
        pats_.push_back({nuisance_type::clicks, std::regex(
            u8"^(\\w+)\\s+\\1(\\s+\\1)*$",
            std::regex::icase | std::regex::ECMAScript)});
        pats_.push_back({nuisance_type::clicks, std::regex(
            "^\\[(click|pop|snap|tap|knock)\\]$",
            std::regex::icase | std::regex::ECMAScript)});
    }

    if (!is_type_disabled(nuisance_type::noise)) {
        utils::log("building noise patterns");
        pats_.push_back({nuisance_type::noise, std::regex(
            "^\\[(breath|inhale|exhale|cough|smack|click|noise|music|дыхание|вдох|выдох|кашель|щелчок|шум|музыка)\\]$",
            std::regex::icase | std::regex::ECMAScript)});
        pats_.push_back({nuisance_type::noise, std::regex(
            "^(breath|inhale|exhale|cough|smack|click|noise|music|дыхание|вдох|выдох|кашель|щелчок|шум|музыка)$",
            std::regex::icase | std::regex::ECMAScript)});
    }

    if (!is_type_disabled(nuisance_type::breath)) {
        utils::log("building breath patterns");
        pats_.push_back({nuisance_type::breath, std::regex(
            u8"^(ah|uh|huh|hm|eh|oh|ha|he|hi|ho|hu)$",
            std::regex::icase | std::regex::ECMAScript)});
        pats_.push_back({nuisance_type::breath, std::regex(
            u8"^[^а-яёА-ЯЁa-zA-Z]*$",
            std::regex::ECMAScript)});
    }
}
