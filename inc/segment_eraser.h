#pragma once

#include "storage.h"

class segment_eraser {
public:
    // returns a new  pcm buffer with the cut ranges removed and fades applied.
    std::vector<float> erase(const std::vector<float>& in, const std::vector<cut_range>& cuts);
};