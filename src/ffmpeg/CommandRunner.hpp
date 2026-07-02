#pragma once

#include <string>
#include <vector>

class CommandRunner
{
public:
    static bool run(const std::string& command, std::string* output = nullptr);
    static bool run(const std::vector<std::string>& args, std::string* output = nullptr);
};
