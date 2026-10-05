#include "FFmpeg.hpp"

#include <cstdlib>

#include "CommandRunner.hpp"

bool FFmpeg::isAvailable()
{
    std::string output;
    return CommandRunner::run("ffmpeg -version | head -n 1", &output);
}

bool FFmpeg::run(const std::string& args, std::string* output)
{
    const std::string command = "ffmpeg " + args;
    return CommandRunner::run(command, output);
}

bool FFmpeg::runWithProgress(
    const std::string& args,
    int durationSeconds,
    const std::function<void(int)>& onProgress)
{
    return CommandRunner::runWithProgress(
        "ffmpeg -progress pipe:1 -nostats " + args,
        durationSeconds,
        onProgress);
}
