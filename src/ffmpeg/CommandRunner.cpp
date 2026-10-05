#include "CommandRunner.hpp"

#include <array>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace
{
std::string readAll(FILE* stream)
{
    std::string output;
    std::array<char, 256> buffer{};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), stream) != nullptr)
    {
        output += buffer.data();
    }
    return output;
}
}

bool CommandRunner::run(const std::string& command, std::string* output)
{
    const std::string mergedCommand = command + " 2>&1";
    FILE* pipe = popen(mergedCommand.c_str(), "r");
    if (!pipe)
    {
        return false;
    }

    std::string result = readAll(pipe);
    if (output != nullptr)
    {
        *output = result;
    }

    const int exitCode = pclose(pipe);
    return exitCode == 0;
}

bool CommandRunner::run(const std::vector<std::string>& args, std::string* output)
{
    if (args.empty())
    {
        return false;
    }

    std::string command;
    for (const auto& arg : args)
    {
        if (command.empty())
        {
            command = arg;
        }
        else
        {
            command += " \"" + arg + "\"";
        }
    }

    return run(command, output);
}

bool CommandRunner::runWithProgress(
    const std::string& command,
    int durationSeconds,
    const std::function<void(int)>& onProgress)
{
    FILE* pipe = popen((command + " 2>&1").c_str(), "r");
    if (!pipe)
    {
        return false;
    }

    int lastReported = -1;
    std::array<char, 1024> buffer{};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
    {
        const std::string_view line(buffer.data());
        constexpr std::string_view key = "out_time_us=";
        const auto position = line.find(key);
        if (position == std::string_view::npos || durationSeconds <= 0 || !onProgress)
        {
            continue;
        }

        const auto valueStart = position + key.size();
        char* end = nullptr;
        const long long elapsedMicros = std::strtoll(buffer.data() + valueStart, &end, 10);
        if (end == buffer.data() + valueStart || elapsedMicros <= 0)
        {
            continue;
        }

        const int percent = static_cast<int>(std::clamp<long long>(
            elapsedMicros * 100 / (static_cast<long long>(durationSeconds) * 1'000'000), 0, 99));
        if (percent > lastReported)
        {
            lastReported = percent;
            onProgress(percent);
        }
    }

    const int exitCode = pclose(pipe);
    if (exitCode == 0 && onProgress && lastReported < 100)
    {
        onProgress(100);
    }
    return exitCode == 0;
}
