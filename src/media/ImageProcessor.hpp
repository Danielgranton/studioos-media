#pragma once

#include <string>
#include "core/Result.hpp"

class ImageProcessor
{
public:
    Result<std::string> compress(const std::string& inputPath, int quality = 85);

    Result<std::string> resize(const std::string& inputPath, int width, int height);

    Result<std::string> thumbnail(const std::string& inputPath, int size = 300);

    Result<std::string> crop(const std::string& inputPath, int x, int y, int width, int height);

    Result<std::string> rotate(const std::string& inputPath, double angle);

    Result<std::string> watermark(const std::string& inputPath, const std::string& watermarkPath, int x, int y);

    Result<std::string> blur(const std::string& inputPath, int kernelSize = 5);

    Result<std::string> sharpen(const std::string& inputPath, double amount = 1.0);

    Result<std::string> removeMetadata(const std::string& inputPath);

    Result<std::string> saveAsWebp(const std::string& inputPath, int quality = 80);

    Result<std::string> saveAsAvif(const std::string& inputPath, int quality = 80);

    Result<std::string> optimizePng(const std::string& inputPath);
};