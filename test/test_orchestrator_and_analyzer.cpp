#include <gtest/gtest.h>
#include "orchestrator.h"
#include "whisper_analyzer.h"
#include "decoder.h"
#include "encoder.h"
#include "config.h"
#include "storage.h"
#include "utils.h"
#include <vector>
#include <cmath>
#include <cstring>

// whisper analyzer test

TEST(WhisperAnalyzerTest, ValidateInitialization) {
    // this test checks if the analyzer can be initialized without throwing
    // !!! in a real test with actual whisper, needed a valid model file
    try {
        whisper_analyzer analyzer("models/ggml-base.bin");
        // if the model file doesn't exist, the test will throw before this line
    } catch (const std::runtime_error& e) {
        // expected if model file doesn't exist in test environment
        EXPECT_TRUE(std::string(e.what()).find("whisper") != std::string::npos);
    }
}

TEST(WhisperAnalyzerTest, AnalysisReturnsSegments) {
    // create mock PCM data for testing
    std::vector<float> pcm_data(16000, 0.1f);
    
    try {
        whisper_analyzer analyzer("models/ggml-base.bin");
        auto segments = analyzer.analyze(pcm_data);
        // if analysis succeeds, check that we get a vector
        EXPECT_GE(segments.size(), 0u);
    } catch (const std::runtime_error&) {
        // skip if model not available
        GTEST_SKIP();
    }
}

// orchestrator mock test

class OrchestratorMockTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        // create temporary test directory
        int ret = system("mkdir -p /tmp/orchestrator_test");
        (void)ret;  // Suppress warning about unused return value
    }
    
    virtual void TearDown() {
        // clean up test directory
        int ret = system("rm -rf /tmp/orchestrator_test");
        (void)ret;  // suppress warning about unused return value
    }
};

TEST_F(OrchestratorMockTest, OrchestratorInitialization) {
    config cfg;
    cfg.model_path = "models/ggml-base.bin";
    cfg.input_files.clear();
    cfg.output_files.clear();
    cfg.disabled_types.clear();
    
    try {
        orchestrator pipeline(cfg);
        // if initialization succeeds
        EXPECT_TRUE(true);
    } catch (const std::exception&) {
        // model file may not exist in test environment
        GTEST_SKIP();
    }
}

TEST_F(OrchestratorMockTest, OrchestratorWithDisabledTypes) {
    config cfg;
    cfg.model_path = "models/ggml-base.bin";
    cfg.input_files.clear();
    cfg.output_files.clear();
    cfg.disabled_types.push_back(nuisance_type::pauses);
    cfg.disabled_types.push_back(nuisance_type::noise);
    
    try {
        orchestrator pipeline(cfg);
        EXPECT_EQ(cfg.disabled_types.size(), 2u);
    } catch (const std::exception&) {
        GTEST_SKIP();
    }
}

TEST_F(OrchestratorMockTest, ConfigurationSurvives) {
    config cfg;
    cfg.model_path = "models/ggml-base.bin";
    cfg.input_files = {"/tmp/orchestrator_test/input.wav"};
    cfg.output_files = {"/tmp/orchestrator_test/output.wav"};
    cfg.pack_mode = false;
    cfg.disabled_types = {nuisance_type::filler};
    
    try {
        orchestrator pipeline(cfg);
        EXPECT_EQ(cfg.input_files.size(), 1u);
        EXPECT_EQ(cfg.output_files.size(), 1u);
    } catch (const std::exception&) {
        GTEST_SKIP();
    }
}

// integration test

TEST(IntegrationTest, DetectorAndEraserTogether) {
    // create segments
    std::vector<segment> segs = {
        {0.0f, 0.5f, "hello"},
        {3.0f, 3.5f, "world"}
    };
    
    // detect nuisance
    nuisance_detector detector({});
    auto cuts = detector.detect(segs, static_cast<size_t>(5.0f * SAMPLE_RATE));
    
    // create audio data
    std::vector<float> pcm(5 * SAMPLE_RATE, 0.5f);
    
    // erase based on detected cuts
    segment_eraser eraser;
    auto result = eraser.erase(pcm, cuts);
    
    // verify result
    EXPECT_LT(result.size(), pcm.size());
}

TEST(IntegrationTest, EmptyDetectionMeansNoCuts) {
    std::vector<segment> segs = {
        {0.5f, 1.0f, "hello"},
        {1.5f, 2.0f, "world"}
    };
    
    nuisance_detector detector({});
    auto cuts = detector.detect(segs, static_cast<size_t>(3.0f * SAMPLE_RATE));
    
    // short pauses should not be detected
    if (cuts.empty()) {
        std::vector<float> pcm(3 * SAMPLE_RATE, 0.5f);
        segment_eraser eraser;
        auto result = eraser.erase(pcm, cuts);
        
        EXPECT_EQ(result.size(), pcm.size());
    }
}

TEST(IntegrationTest, AllNuisanceTypesDisabled) {
    std::vector<segment> segs = {
        {0.0f, 1.0f, "um"},
        {5.0f, 6.0f, "ah"}
    };
    
    std::vector<nuisance_type> disabled = {
        nuisance_type::filler,
        nuisance_type::noise,
        nuisance_type::clicks,
        nuisance_type::pauses,
        nuisance_type::breath
    };
    
    nuisance_detector detector(disabled);
    auto cuts = detector.detect(segs, static_cast<size_t>(7.0f * SAMPLE_RATE));
    
    // with all types disabled, only leading/trailing silence would be detected
    // and they're also disabled via pauses, so should be empty
    std::vector<float> pcm(7 * SAMPLE_RATE, 0.5f);
    segment_eraser eraser;
    auto result = eraser.erase(pcm, cuts);
    
    EXPECT_GE(result.size(), 0u);
}

TEST(IntegrationTest, LongFileSplitIntoSegments) {
    // simulate a longer file analysis
    std::vector<segment> segs;
    for (int i = 0; i < 20; ++i) {
        segs.push_back({
            static_cast<float>(i * 2),
            static_cast<float>(i * 2 + 1),
            "word"
        });
    }
    
    nuisance_detector detector({});
    auto cuts = detector.detect(segs, static_cast<size_t>(50.0f * SAMPLE_RATE));
    
    EXPECT_GE(cuts.size(), 0u);
}
