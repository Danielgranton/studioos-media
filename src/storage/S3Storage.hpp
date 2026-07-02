#pragma once

#include <string>
#include <utility>

#include "core/Result.hpp"

class S3Storage
{
public:
    struct ParsedReference
    {
        std::string bucket;
        std::string key;
    };

    bool configured() const;

    Result<std::string> uploadFile(
        const std::string& localPath,
        const std::string& objectKey = "");

    Result<std::string> downloadFile(
        const std::string& s3Reference,
        const std::string& localPath);

    static bool isS3Reference(const std::string& reference);
    static Result<ParsedReference> parseReference(const std::string& reference);
    std::string makeS3Reference(const std::string& objectKey) const;

private:
    std::string buildBaseArgs() const;
    std::string buildCopyCommand(
        const std::string& source,
        const std::string& destination) const;
};
