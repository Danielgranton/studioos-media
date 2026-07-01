#pragma once

#include <string>

#include "media/ImageProcessor.hpp"

class ImageService
{
private:

    ImageProcessor processor;

public:

    std::string compress(const std::string& path)
    {
        return processor.compress(path);
    }

    std::string resize(
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

    std::string thumbnail(
        const std::string& path)
    {
        return processor.thumbnail(path);
    }
};