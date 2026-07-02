#include "CommandRunner.hpp"

#include <array>
#include <cstdio>

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
