#pragma once

#include "storage.h"

class nuisance_detector {
    public:
    // returns list of time ranges to cut
    std::vector<cut_range> detect(const std::vector<segment>& segs,
                                  size_t total_samples);
};