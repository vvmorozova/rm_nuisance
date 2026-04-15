#include "encoder.h"
#include "utils.h"
#include <stdexcept>
#include <regex>

void encoder::encode(const std::vector<float>& pcm, const std::string& output_path)
{
    utils::log("encoder: writing '" + output_path + "' …");
    
    // build ffmpeg command that reads f32le from stdin and encodes to output.
    // probe the input to keep original codec family where possible.
    std::string cmd =
        "ffmpeg -hide_banner -loglevel error -y"
        " -f f32le -ar 16000 -ac 1 -i pipe:0"  // pcm input from stdin
        " " + encode_flags(output_path) +
        " " + shell_quote(output_path);

    FILE* pipe = popen(cmd.c_str(), "w");
    if (!pipe)
        throw std::runtime_error("encoder: failed to launch ffmpeg: " + cmd);

    size_t written = fwrite(pcm.data(), sizeof(float), pcm.size(), pipe);
    if (written != pcm.size())
        throw std::runtime_error("encoder: short write (" +
            std::to_string(written) + "/" + std::to_string(pcm.size()) + ")");

    int ret = pclose(pipe);
    if (ret != 0)
        throw std::runtime_error("encoder: ffmpeg exited with code " + std::to_string(ret));

    utils::log("encoder: done");
}

std::string encoder::encode_flags(const std::string& path) {
    std::string ext;
    auto dot = path.rfind('.');
    if (dot != std::string::npos)
        ext = path.substr(dot + 1);
    for (auto& c : ext) c = static_cast<char>(tolower(c));

    if (ext == "mp3")  return "-c:a libmp3lame -q:a 2";
    if (ext == "ogg")  return "-c:a libvorbis -q:a 6";
    if (ext == "opus") return "-c:a libopus -b:a 128k";
    if (ext == "m4a" || ext == "aac") return "-c:a aac -b:a 192k";
    if (ext == "flac") return "-c:a flac";
    if (ext == "wav")  return "-c:a pcm_s16le";
    // default: let ffmpeg decide
    return "";
}

std::string encoder::shell_quote(const std::string& s) {
    return "'" + std::regex_replace(s, std::regex("'"), "'\\''") + "'";
}