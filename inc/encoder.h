#pragma once
#include <vector>
#include <string>

class encoder {
public:
    void encode(const std::vector<float>& pcm, const std::string& output_path);
private:
    std::string encode_flags(const std::string& path);

    std::string shell_quote(const std::string& s);
};