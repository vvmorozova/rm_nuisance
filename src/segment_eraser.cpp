#include "segment_eraser.h"
#include "utils.h"
#include <algorithm>

std::vector<float> segment_eraser::erase(const std::vector<float>& in, std::vector<cut_range>& cuts)
{
    if (cuts.empty()) {
        utils::log("eraser: nothing to cut, passing through");
        return in;
    }

    utils::log("eraser: cutting " + std::to_string(cuts.size()) + " segment(s)");
    for (size_t i = 0; i < cuts.size(); ++i) {
        utils::log("eraser: cut " + std::to_string(i) + ": " + std::to_string(cuts[i].start_s) + " - " + std::to_string(cuts[i].end_s));
    }

    const int fade_samples = static_cast<int>(FADE_DURATION_S * SAMPLE_RATE);

    // build list of "keep" intervals (complement of cuts)
    std::vector<std::pair<size_t,size_t>> keep_ranges;
    size_t cursor = 0;

    std::sort(cuts.begin(), cuts.end(), [](cut_range r1, cut_range r2) -> bool {
        return r1.start_s < r2.start_s;
    });

    for (const auto& cut : cuts) {
        size_t cut_start = clamp_sample(cut.start_s, in.size());
        size_t cut_end   = clamp_sample(cut.end_s,   in.size());

        if (cut_start > cursor) {
            keep_ranges.push_back({cursor, cut_start});
        }

        cursor = cut_end;
    }
    if (cursor < in.size()) {
        keep_ranges.push_back({cursor, in.size()});
    }

    // copy kept segments into output with per-segment fade
    std::vector<float> out;
    out.reserve(in.size());

    for (size_t ki = 0; ki < keep_ranges.size(); ++ki) {
        auto s = keep_ranges[ki].first;
        auto e = keep_ranges[ki].second;
        
        if (s >= e) {
            continue;
        }

        const size_t output_start = out.size();
        out.insert(out.end(), in.begin() + s, in.begin() + e);
        auto it_begin = out.begin() + output_start;
        auto it_end = out.end();

        // fade-in at the beginning of every keep segment (except the very first)
        if (ki > 0) {
            apply_fade_in(it_begin, it_end, fade_samples);
        }

        // fade-out at the end of every keep segment (except the very last)
        if (ki + 1 < keep_ranges.size()) {
            apply_fade_out(it_begin, it_end, fade_samples);
        }

        //out.insert(out.end(), seg.begin(), seg.end());
    }

    float saved_s = static_cast<float>(in.size() - out.size()) / SAMPLE_RATE;
    utils::log("eraser: removed " + std::to_string(saved_s) + " s of audio");
    utils::log("eraser: output length " +
        std::to_string(static_cast<float>(out.size()) / SAMPLE_RATE) + " s");

    return out;
}

size_t segment_eraser::clamp_sample(float t, size_t max_sz) {
    long long n = static_cast<long long>(t * SAMPLE_RATE);
    if (n < 0) {
        n = 0;
    }
    if (static_cast<size_t>(n) > max_sz) {
        n = static_cast<long long>(max_sz);
    }
    return static_cast<size_t>(n);
}

void segment_eraser::apply_fade_in(std::vector<float>::iterator& it_begin,
    std::vector<float>::iterator& it_end, int n)
{
    int len = std::min(n, static_cast<int>(std::distance(it_begin, it_end)) / 2);
    for (int i = 0; i < len; ++i) {
        *std::next(it_begin, i) *= static_cast<float>(i) / len;
    }
}

void segment_eraser::apply_fade_out(std::vector<float>::iterator& it_begin,
    std::vector<float>::iterator& it_end, int n)
{
    int total = static_cast<int>(std::distance(it_begin, it_end));
    int len   = std::min(n, total / 2);
    int start = total - len;
    for (int i = start; i < total; ++i) {
        *std::next(it_begin, i) *= static_cast<float>(total - 1 - i) / len;
    }
}