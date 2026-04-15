#pragma once

#include "decoder.h"
#include "whisper_analyzer.h"
#include "nuisance_detector.h"
#include "segment_eraser.h"
#include "encoder.h"
#include "config.h"

class orchestrator {
    decoder             decoder_;
    whisper_analyzer    whisper_;
    nuisance_detector   detector_;
    segment_eraser      eraser_;
    encoder             encoder_;
    config              cfg_;
public:
    explicit orchestrator(const config cfg) : whisper_(cfg.model_path), cfg_(cfg){};
 
    void run();
private:
    std::string generate_output_name(std::string input);
};