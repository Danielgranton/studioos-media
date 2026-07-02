#include "ImageService.hpp"

Result<std::string> ImageService::compress(const std::string& path)
{
    return processor.compress(path);
}

Result<std::string> ImageService::resize(const std::string& path, int width, int height)
{
    return processor.resize(path, width, height);
}

Result<std::string> ImageService::thumbnail(const std::string& path)
{
    return processor.thumbnail(path);
}

Result<std::string> ImageService::crop(const std::string& path, int x, int y, int width, int height)
{
    return processor.crop(path, x, y, width, height);
}

Result<std::string> ImageService::rotate(const std::string& path, double angle)
{
    return processor.rotate(path, angle);
}

Result<std::string> ImageService::watermark(const std::string& path, const std::string& watermarkPath, int x, int y)
{
    return processor.watermark(path, watermarkPath, x, y);
}

Result<std::string> ImageService::blur(const std::string& path, int kernelSize)
{
    return processor.blur(path, kernelSize);
}

Result<std::string> ImageService::sharpen(const std::string& path, double amount)
{
    return processor.sharpen(path, amount);
}

Result<std::string> ImageService::removeMetadata(const std::string& path)
{
    return processor.removeMetadata(path);
}

Result<std::string> ImageService::saveAsWebp(const std::string& path, int quality)
{
    return processor.saveAsWebp(path, quality);
}

Result<std::string> ImageService::saveAsAvif(const std::string& path, int quality)
{
    return processor.saveAsAvif(path, quality);
}

Result<std::string> ImageService::optimizePng(const std::string& path)
{
    return processor.optimizePng(path);
}