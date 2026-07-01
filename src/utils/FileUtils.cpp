#include "FileUtils.hpp"

#include <chrono>
#include <fstream>
#include <random>

namespace fs = std::filesystem;

bool FileUtils::exists(const std::string& path)
{
    return fs::exists(path);
}

bool FileUtils::createDirectory(const std::string& path)
{
    return fs::create_directories(path);
}

bool FileUtils::deleteFile(const std::string& path)
{
    return fs::remove(path);
}

bool FileUtils::copyFile(const std::string& from, const std::string& destination)
{
    try
    {
        fs::copy_file(from, destination, fs::copy_options::overwrite_existing);
        return true;
    }
    catch (...) {
        return false;
    }
}

bool FileUtils::moveFile(const std::string& from, const std::string& to)
{
    try
    {
        fs::rename(from, to);
        return true;
    }
    catch (...) {
        return false;
    }
}

std::string FileUtils::filename(const std::string& path)
{
    return fs::path(path).filename().string();
}

std::string FileUtils::extension(const std::string& path)
{
    return fs::path(path).extension().string();
}

std::string FileUtils::stem(const std::string& path)
{
    return fs::path(path).stem().string();
}

std::size_t FileUtils::fileSize(const std::string& path)
{
    if (!exists(path))
    {
        return 0;
    }

    return static_cast<std::size_t>(fs::file_size(path));
}

std::string FileUtils::tempFile(const std::string& prefix)
{
    static std::random_device device;
    static std::mt19937 generator(device());
    static std::uniform_int_distribution<int> distribution(100000, 999999);

    const std::string output = prefix + std::to_string(distribution(generator));
    std::ofstream stream(output, std::ios::binary);
    stream.close();
    return output;
}