#pragma once

class encoder {
public:
    void encode(const std::vector<float>& pcm,
                const std::string& input_path,
                const std::string& output_path);
};