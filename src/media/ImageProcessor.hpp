#pragma once

#include <string>
#include "core/Result.hpp"

class ImageProcessor
{
public:

    // Compress an image
    Result<std::string> compress(
        const std::string& inputPath,
        int quality = 85
    );

    // Resize an image
    Result<std::string> resize(
        const std::string& inputPath,
        int width,
        int height
    );

    // Generate a thumbnail
    Result<std::string> thumbnail(
        const std::string& inputPath,
        int size = 300
    );
};