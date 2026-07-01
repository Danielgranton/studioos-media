#pragma once

#include <string>

#include "core/Result.hpp"
#include "media/ImageProcessor.hpp"

class ImageService
{
public:

    Result<std::string> compress(
        const std::string& path
    );

    Result<std::string> resize(
        const std::string& path,
        int width,
        int height
    );

    Result<std::string> thumbnail(
        const std::string& path
    );

private:

    ImageProcessor processor;
};