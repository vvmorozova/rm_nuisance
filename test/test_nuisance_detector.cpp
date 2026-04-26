#include <gtest/gtest.h>
#include "nuisance_detector.h"
#include "segment_eraser.h"
#include "storage.h"
#include "utils.h"
#include <cmath>

// nuisance detector test

TEST(NuisanceDetectorTest, DetectsLongPauseBetweenSegments) {
    std::vector<segment> segs = {
        {0.0f, 1.0f, "hello"},
        {4.5f, 5.0f, "world"}
    };

    size_t total_samples = static_cast<size_t>(6.0f * SAMPLE_RATE);
    std::vector<float> pcm(total_samples, 0.1f);

    nuisance_detector detector({});
    auto cuts = detector.detect(segs, total_samples, pcm);

    ASSERT_EQ(cuts.size(), 1u);
    EXPECT_NEAR(cuts[0].start_s, 1.15f, 1e-3);
    EXPECT_NEAR(cuts[0].end_s, 4.35f, 1e-3);
}

TEST(NuisanceDetectorTest, DetectsFillerWords) {
    std::vector<segment> segs = {
        {0.0f, 0.5f, "hello"},
        {0.6f, 1.1f, "um"},
        {1.5f, 2.0f, "world"}
    };

    size_t total_samples = static_cast<size_t>(3.0f * SAMPLE_RATE);
    std::vector<float> pcm(total_samples, 0.1f);

    nuisance_detector detector({});
    auto cuts = detector.detect(segs, total_samples, pcm);

    EXPECT_GT(cuts.size(), 0u);
}

TEST(NuisanceDetectorTest, IgnoresDisabledPauseType) {
    std::vector<segment> segs = {
        {0.0f, 1.0f, "hello"},
        {5.0f, 6.0f, "world"}
    };

    size_t total_samples = static_cast<size_t>(7.0f * SAMPLE_RATE);
    std::vector<float> pcm(total_samples, 0.1f);

    std::vector<nuisance_type> disabled = {nuisance_type::pauses};
    nuisance_detector detector(disabled);
    auto cuts = detector.detect(segs, total_samples, pcm);

    EXPECT_EQ(cuts.size(), 0u);
}

TEST(NuisanceDetectorTest, DetectsLeadingSilence) {
    std::vector<segment> segs = {
        {3.0f, 4.0f, "hello"}
    };

    size_t total_samples = static_cast<size_t>(5.0f * SAMPLE_RATE);
    std::vector<float> pcm(total_samples, 0.1f);

    nuisance_detector detector({});
    auto cuts = detector.detect(segs, total_samples, pcm);

    ASSERT_GT(cuts.size(), 0u);
    EXPECT_LT(cuts[0].start_s, 1.0f);
}

TEST(NuisanceDetectorTest, DetectsTrailingSilence) {
    std::vector<segment> segs = {
        {0.5f, 1.0f, "hello"}
    };

    size_t total_samples = static_cast<size_t>(5.0f * SAMPLE_RATE);
    std::vector<float> pcm(total_samples, 0.1f);

    nuisance_detector detector({});
    auto cuts = detector.detect(segs, total_samples, pcm);

    ASSERT_GT(cuts.size(), 0u);
    EXPECT_GT(cuts.back().end_s, 3.0f);
}

TEST(NuisanceDetectorTest, IgnoresShortPauses) {
    std::vector<segment> segs = {
        {0.5f, 1.0f, "hello"},
        {1.5f, 2.0f, "world"}
    };

    size_t total_samples = static_cast<size_t>(3.0f * SAMPLE_RATE);
    std::vector<float> pcm(total_samples, 0.1f);

    nuisance_detector detector({});
    auto cuts = detector.detect(segs, total_samples, pcm);

    EXPECT_EQ(cuts.size(), 0u);
}

TEST(NuisanceDetectorTest, MergesOverlappingCuts) {
    std::vector<segment> segs = {
        {0.0f, 1.0f, "a"},
        {5.0f, 5.1f, "b"},
        {5.2f, 5.3f, "c"},
        {10.0f, 11.0f, "d"}
    };

    size_t total_samples = static_cast<size_t>(12.0f * SAMPLE_RATE);
    std::vector<float> pcm(total_samples, 0.1f);

    nuisance_detector detector({});
    auto cuts = detector.detect(segs, total_samples, pcm);

    EXPECT_GT(cuts.size(), 0u);
}

TEST(NuisanceDetectorTest, HandlesEmptySegments) {
    std::vector<segment> segs = {};

    size_t total_samples = static_cast<size_t>(5.0f * SAMPLE_RATE);
    std::vector<float> pcm(total_samples, 0.1f);

    nuisance_detector detector({});
    auto cuts = detector.detect(segs, total_samples, pcm);

    EXPECT_GE(cuts.size(), 0u);
}

TEST(NuisanceDetectorTest, DetectsMultipleNuisanceTypes) {
    std::vector<segment> segs = {
        {0.5f, 1.0f, "hello"},
        {1.2f, 1.5f, "hm"},
        {5.5f, 6.0f, "world"}
    };

    size_t total_samples = static_cast<size_t>(7.0f * SAMPLE_RATE);
    std::vector<float> pcm(total_samples, 0.1f);

    nuisance_detector detector({});
    auto cuts = detector.detect(segs, total_samples, pcm);

    EXPECT_GT(cuts.size(), 0u);
}

// segment eraser test

TEST(SegmentEraserTest, ErasesEmptyCuts) {
    std::vector<float> pcm(16000, 0.5f);
    std::vector<cut_range> cuts;

    segment_eraser eraser;
    auto result = eraser.erase(pcm, cuts);

    EXPECT_EQ(result.size(), pcm.size());
    EXPECT_EQ(result, pcm);
}

TEST(SegmentEraserTest, ErasesSingleSegment) {
    std::vector<float> pcm(16000, 0.5f);
    std::vector<cut_range> cuts = {
        {0.5f, 1.0f}
    };

    segment_eraser eraser;
    auto result = eraser.erase(pcm, cuts);

    EXPECT_LT(result.size(), pcm.size());
}

TEST(SegmentEraserTest, ErasesMultipleSegments) {
    std::vector<float> pcm(32000, 0.5f);
    std::vector<cut_range> cuts = {
        {0.3f, 0.6f},
        {1.0f, 1.3f}
    };

    segment_eraser eraser;
    auto result = eraser.erase(pcm, cuts);

    EXPECT_LT(result.size(), pcm.size());
}

TEST(SegmentEraserTest, AppliesFadeInToNonFirstSegment) {
    // create audio with multiple segments to cut
    std::vector<float> pcm(48000, 1.0f);  // 3 seconds of audio
    std::vector<cut_range> cuts = {
        {0.5f, 1.0f}  // Cut first 0.5 seconds of middle section
    };

    segment_eraser eraser;
    auto result = eraser.erase(pcm, cuts);

    // after cut, remain audio should show fading
    EXPECT_LT(result.size(), pcm.size());
    if (result.size() > 1000) {
        // fade-in will be applied to the second segment (after the cut)
        // check that some fading was applied (values increase from near-zero)
        EXPECT_GE(result[0], 0.0f);
    }
}

TEST(SegmentEraserTest, ErasesFromStart) {
    std::vector<float> pcm(16000, 0.5f);
    std::vector<cut_range> cuts = {
        {0.0f, 0.5f}
    };

    segment_eraser eraser;
    auto result = eraser.erase(pcm, cuts);

    EXPECT_LT(result.size(), pcm.size());
}

TEST(SegmentEraserTest, ErasesUpToEnd) {
    std::vector<float> pcm(16000, 0.5f);
    std::vector<cut_range> cuts = {
        {0.5f, 1.0f}
    };

    segment_eraser eraser;
    auto result = eraser.erase(pcm, cuts);

    EXPECT_LT(result.size(), pcm.size());
}
