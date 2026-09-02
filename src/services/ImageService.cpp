#include "ImageService.hpp"

#include <filesystem>
#include <algorithm>
#include <vector>

#include "core/StatusCode.hpp"
#include "config/config.hpp"
#include "storage/S3Storage.hpp"
#include "utils/FileUtils.hpp"

namespace
{
bool isManagedTemporaryFile(const std::string& path)
{
    const std::string prefix = Config::instance().tempFolder() + "/";
    return path.rfind(prefix, 0) == 0;
}

Result<std::string> uploadVariant(S3Storage& storage, const std::string& localPath, const std::string& objectKey)
{
    if (!storage.configured())
    {
        return Result<std::string>::ok(localPath);
    }

    return storage.uploadFile(localPath, objectKey);
}
}

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

Result<ResponsiveImageResult> ImageService::processResponsiveImage(
    const std::string& path,
    const std::string& objectKeyPrefix,
    int quality)
{
    if (path.empty())
    {
        return Result<ResponsiveImageResult>::fail("Image path is required", StatusCode::INVALID_FORMAT);
    }

    S3Storage storage;
    const std::vector<int> sizes = {1024, 512, 256, 128, 64};
    std::vector<std::string> temporaryFiles;
    std::vector<std::string> localOutputs;
    auto cleanup = [&temporaryFiles, &localOutputs](bool removeOutputs) {
        for (const auto& file : temporaryFiles)
        {
            if (!removeOutputs && std::find(localOutputs.begin(), localOutputs.end(), file) != localOutputs.end()) {
                continue;
            }
            if (isManagedTemporaryFile(file)) FileUtils::deleteFile(file);
        }
    };
    auto fail = [&cleanup](const std::string& message, StatusCode status) {
        cleanup(true);
        return Result<ResponsiveImageResult>::fail(message, status);
    };
    if (isManagedTemporaryFile(path)) temporaryFiles.push_back(path);

    auto normalized = processor.removeMetadata(path);
    if (!normalized.success)
    {
        return fail(normalized.message, normalized.status);
    }
    temporaryFiles.push_back(normalized.value);

    auto originalWebp = processor.saveAsWebp(normalized.value, quality);
    if (!originalWebp.success)
    {
        return fail(originalWebp.message, originalWebp.status);
    }
    temporaryFiles.push_back(originalWebp.value);
    if (!storage.configured()) localOutputs.push_back(originalWebp.value);

    ResponsiveImageResult result;
    auto originalUpload = uploadVariant(storage, originalWebp.value, objectKeyPrefix + "/original.webp");
    if (!originalUpload.success)
    {
        return fail(originalUpload.message, originalUpload.status);
    }
    result.originalUrl = originalUpload.value;

    for (int size : sizes)
    {
        auto square = processor.squareThumbnail(normalized.value, size);
        if (!square.success)
        {
            return fail(square.message, square.status);
        }
        temporaryFiles.push_back(square.value);

        auto webp = processor.saveAsWebp(square.value, quality);
        if (!webp.success)
        {
            return fail(webp.message, webp.status);
        }
        temporaryFiles.push_back(webp.value);
        if (!storage.configured()) localOutputs.push_back(webp.value);

        const std::string objectKey = objectKeyPrefix + "/" + std::to_string(size) + ".webp";
        auto uploaded = uploadVariant(storage, webp.value, objectKey);
        if (!uploaded.success)
        {
            return Result<ResponsiveImageResult>::fail(uploaded.message, uploaded.status);
        }

        result.variants.push_back(ResponsiveImageVariant{size, uploaded.value});
    }

    if (!storage.configured() && !FileUtils::exists(result.originalUrl))
    {
        return fail("Responsive image outputs were not created", StatusCode::INTERNAL_ERROR);
    }

    cleanup(storage.configured());
    return Result<ResponsiveImageResult>::ok(result);
}
