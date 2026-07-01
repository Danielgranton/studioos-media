#include "ImageService.hpp"

std::string ImageService::compress(const std::string& path)
{
    return processor.compress(path);
}