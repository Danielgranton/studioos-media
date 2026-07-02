#pragma once

#include <string>

#include "core/Result.hpp"
#include "media/ImageProcessor.hpp"

class ImageService
{
public:
    Result<std::string> compress(const std::string& path);

    Result<std::string> resize(const std::string& path, int width, int height);

    Result<std::string> thumbnail(const std::string& path);

    Result<std::string> crop(const std::string& path, int x, int y, int width, int height);

    Result<std::string> rotate(const std::string& path, double angle);

    Result<std::string> watermark(const std::string& path, const std::string& watermarkPath, int x, int y);

    Result<std::string> blur(const std::string& path, int kernelSize = 5);

    Result<std::string> sharpen(const std::string& path, double amount = 1.0);

    Result<std::string> removeMetadata(const std::string& path);

    Result<std::string> saveAsWebp(const std::string& path, int quality = 80);

    Result<std::string> saveAsAvif(const std::string& path, int quality = 80);

    Result<std::string> optimizePng(const std::string& path);

private:
    ImageProcessor processor;
};