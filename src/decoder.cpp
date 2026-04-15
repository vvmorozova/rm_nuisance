#include "decoder.h"
#include "utils.h"
#include <regex>

std::vector<float> decoder::decode(const std::string& path) {
    utils::log("Decoder: opening '" + path + "'");

    std::string cmd =
        "ffmpeg -hide_banner -loglevel error"
        " -i " + shell_quote(path) +
        " -ar 16000 -ac 1 -f f32le -";  // output raw f32le to stdout

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        throw std::runtime_error("Decoder: failed to launch ffmpeg: " + cmd);

    std::vector<float> samples;
    float sample;
    while (fread(&sample, sizeof(float), 1, pipe) == 1)
        samples.push_back(sample);

    int ret = pclose(pipe);
    if (ret != 0)
        throw std::runtime_error("Decoder: ffmpeg exited with code " + std::to_string(ret));

    utils::log("Decoder: got " + std::to_string(samples.size()) +
        " samples (" + fmt_duration(samples.size()) + ")");
    return samples;
}

std::string decoder::shell_quote(const std::string& s) {
    return "'" + std::regex_replace(s, std::regex("'"), "'\\''") + "'";
}

std::string decoder::fmt_duration(size_t n) {
    float sec = static_cast<float>(n) / SAMPLE_RATE;
    int m = static_cast<int>(sec) / 60;
    int s = static_cast<int>(sec) % 60;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d:%02d", m, s);
    return buf;
}