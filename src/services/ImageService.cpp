#include "ImageService.hpp"

#include "core/StatusCode.hpp"
#include "storage/S3Storage.hpp"
#include "utils/FileUtils.hpp"

namespace
{
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

    auto normalized = processor.removeMetadata(path);
    if (!normalized.success)
    {
        return Result<ResponsiveImageResult>::fail(normalized.message, normalized.status);
    }

    auto originalWebp = processor.saveAsWebp(normalized.value, quality);
    if (!originalWebp.success)
    {
        return Result<ResponsiveImageResult>::fail(originalWebp.message, originalWebp.status);
    }

    ResponsiveImageResult result;
    auto originalUpload = uploadVariant(storage, originalWebp.value, objectKeyPrefix + "/original.webp");
    if (!originalUpload.success)
    {
        return Result<ResponsiveImageResult>::fail(originalUpload.message, originalUpload.status);
    }
    result.originalUrl = originalUpload.value;

    for (int size : sizes)
    {
        auto square = processor.squareThumbnail(normalized.value, size);
        if (!square.success)
        {
            return Result<ResponsiveImageResult>::fail(square.message, square.status);
        }

        auto webp = processor.saveAsWebp(square.value, quality);
        if (!webp.success)
        {
            return Result<ResponsiveImageResult>::fail(webp.message, webp.status);
        }

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
        return Result<ResponsiveImageResult>::fail("Responsive image outputs were not created", StatusCode::INTERNAL_ERROR);
    }

    return Result<ResponsiveImageResult>::ok(result);
}
