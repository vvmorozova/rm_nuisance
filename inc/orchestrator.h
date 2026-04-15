#pragma once

#include "decoder.h"
#include "whisper_analyzer.h"
#include "nuisance_detector.h"
#include "segment_eraser.h"
#include "encoder.h"

class orchestrator {
    decoder             decoder_;
    whisper_analyzer    whisper_;
    nuisance_detector   detector_;
    segment_eraser      eraser_;
    encoder             encoder_;
public:
    explicit orchestrator(const std::string& model_path) : whisper_(model_path) {}
 
    void run(const std::string& input_path, const std::string& output_path);
};