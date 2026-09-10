#pragma once

#include <string>
#include <sstream>
#include <iomanip>

namespace miqumusic {

inline std::string format_duration(unsigned int seconds) {
    unsigned int mins = seconds / 60;
    unsigned int secs = seconds % 60;
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << mins << ":"
        << std::setfill('0') << std::setw(2) << secs;
    return oss.str();
}

inline std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n\"'");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n\"'");
    return str.substr(first, last - first + 1);
}

} // namespace miqumusic
