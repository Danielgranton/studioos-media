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
