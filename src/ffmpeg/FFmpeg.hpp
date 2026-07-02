#pragma once

#include <string>

class FFmpeg
{
public:
    static bool isAvailable();
    static bool run(const std::string& args, std::string* output = nullptr);
};
