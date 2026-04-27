#pragma once

#include "storage.h"
#include <vector>

class segment_eraser {
public:
    // returns a new  pcm buffer with the cut ranges removed and fades applied.
    std::vector<float> erase(const std::vector<float>& in, std::vector<cut_range>& cuts);

private:
    size_t clamp_sample(float t, size_t max_sz);
 
    // linear fade-in over first `n` samples
    void apply_fade_in(std::vector<float>::iterator& it_begin, 
                        std::vector<float>::iterator& it_end, int n);
 
    // linear fade-out over last `n` samples
    void apply_fade_out(std::vector<float>::iterator& it_begin, 
                        std::vector<float>::iterator& it_end, int n);
};