#pragma once

#include <string_view>

#define BoolToString(x) ((x) ? "true" : "false")

inline bool isStaticFilePath(std::string_view path)
{
    constexpr std::string_view prefix = "/static/";
    if (path.size() <= prefix.size() || path.substr(0, prefix.size()) != prefix)
        return false;

    for (unsigned char c : path) {
        if (c < ' ' || c == 0x7f || c == '\\')
            return false;
    }

    size_t start = prefix.size();
    while (start < path.size()) {
        const size_t end = path.find('/', start);
        const auto segment = path.substr(start, end == std::string_view::npos ? end : end - start);
        if (segment.empty() || segment == "." || segment == "..")
            return false;
        if (end == std::string_view::npos)
            return true;
        start = end + 1;
    }
    return false;
}
