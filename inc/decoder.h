#include <vector>
#include <string>

class decoder {
public:
    std::vector<float> decode(const std::string& path);
 
private:
    static std::string shell_quote(const std::string& s);
 
    static std::string fmt_duration(size_t n);
};