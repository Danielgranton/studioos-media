#pragma once

#include <filesystem>
#include <string>

class FileUtils
{
public:
    static bool exists(const std::string& path);
    static bool createDirectory(const std::string& path);
    static bool deleteFile(const std::string& path);
    static bool moveFile(const std::string& from, const std::string& to);
    static bool copyFile(const std::string& from, const std::string& destination);
    static std::string extension(const std::string& path);
    static std::string filename(const std::string& path);
    static std::string stem(const std::string& path);
    static std::size_t fileSize(const std::string& path);
    static std::string tempFile(const std::string& prefix);
};