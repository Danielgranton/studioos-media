#include "ImageService.hpp"

Result<std::string> ImageService::compress(
    const std::string& path)
{
    return processor.compress(path);
}

Result<std::string> ImageService::resize(
    const std::string& path,
    int width,
    int height)
{
    return processor.resize(
        path,
        width,
        height
    );
}

Result<std::string> ImageService::thumbnail(
    const std::string& path)
{
    return processor.thumbnail(path);
}