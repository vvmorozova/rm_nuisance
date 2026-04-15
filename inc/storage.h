#pragma once

#include <string>

struct segment {
    float start_s;
    float end_s;
    std::string text;
};
 
struct cut_range {
    float start_s;
    float end_s;
};