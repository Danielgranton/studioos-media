#pragma once

#include <string>

class ImageProcessor
{
public:

    // Compress an image
    std::string compress(
        const std::string& inputPath,
        int quality = 85
    );

    // Resize an image
    std::string resize(
        const std::string& inputPath,
        int width,
        int height
    );

    // Generate a thumbnail
    std::string thumbnail(
        const std::string& inputPath,
        int size = 300
    );
};