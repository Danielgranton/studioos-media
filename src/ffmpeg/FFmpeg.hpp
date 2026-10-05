#pragma once

#include <string>
#include <functional>

class FFmpeg
{
public:
    static bool isAvailable();
    static bool run(const std::string& args, std::string* output = nullptr);
    static bool runWithProgress(
        const std::string& args,
        int durationSeconds,
        const std::function<void(int)>& onProgress);
};
