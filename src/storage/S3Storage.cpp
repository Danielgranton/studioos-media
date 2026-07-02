#include "S3Storage.hpp"

#include <filesystem>
#include <sstream>

#include "config/config.hpp"
#include "ffmpeg/CommandRunner.hpp"
#include "utils/FileUtils.hpp"

namespace
{
std::string shellQuote(const std::string& value)
{
    std::string quoted = "'";
    for (char ch : value)
    {
        if (ch == '\'')
        {
            quoted += "'\\''";
        }
        else
        {
            quoted += ch;
        }
    }
    quoted += "'";
    return quoted;
}

std::string trimSlashes(const std::string& value)
{
    const auto start = value.find_first_not_of('/');
    if (start == std::string::npos)
    {
        return "";
    }

    const auto end = value.find_last_not_of('/');
    return value.substr(start, end - start + 1);
}

std::string joinKey(const std::string& prefix, const std::string& key)
{
    const std::string cleanPrefix = trimSlashes(prefix);
    const std::string cleanKey = trimSlashes(key);
    if (cleanPrefix.empty())
    {
        return cleanKey;
    }

    if (cleanKey.empty())
    {
        return cleanPrefix;
    }

    return cleanPrefix + "/" + cleanKey;
}
}

bool S3Storage::configured() const
{
    return !Config::instance().s3Bucket().empty();
}

Result<std::string> S3Storage::uploadFile(
    const std::string& localPath,
    const std::string& objectKey)
{
    if (!configured())
    {
        return Result<std::string>::fail("S3 bucket is not configured", StatusCode::INVALID_FORMAT);
    }

    if (localPath.empty() || !FileUtils::exists(localPath))
    {
        return Result<std::string>::fail("Local upload file not found", StatusCode::FILE_NOT_FOUND);
    }

    const std::string key = objectKey.empty()
        ? FileUtils::filename(localPath)
        : objectKey;
    const std::string destination = makeS3Reference(key);

    std::string output;
    if (!CommandRunner::run(buildCopyCommand(localPath, destination), &output))
    {
        return Result<std::string>::fail(
            output.empty() ? "Failed to upload file to S3" : output,
            StatusCode::INTERNAL_ERROR);
    }

    return Result<std::string>::ok(destination);
}

Result<std::string> S3Storage::downloadFile(
    const std::string& s3Reference,
    const std::string& localPath)
{
    if (localPath.empty())
    {
        return Result<std::string>::fail("Local download path is required", StatusCode::INVALID_FORMAT);
    }

    auto parsed = parseReference(s3Reference);
    if (!parsed.success)
    {
        return Result<std::string>::fail(parsed.message, parsed.status);
    }

    const std::string parent = std::filesystem::path(localPath).parent_path().string();
    if (!parent.empty())
    {
        FileUtils::createDirectory(parent);
    }

    std::string output;
    if (!CommandRunner::run(buildCopyCommand(s3Reference, localPath), &output))
    {
        return Result<std::string>::fail(
            output.empty() ? "Failed to download file from S3" : output,
            StatusCode::INTERNAL_ERROR);
    }

    return Result<std::string>::ok(localPath);
}

bool S3Storage::isS3Reference(const std::string& reference)
{
    return reference.rfind("s3://", 0) == 0;
}

Result<S3Storage::ParsedReference> S3Storage::parseReference(const std::string& reference)
{
    if (!isS3Reference(reference))
    {
        return Result<ParsedReference>::fail("S3 reference must start with s3://", StatusCode::INVALID_FORMAT);
    }

    const std::string remainder = reference.substr(5);
    const auto slash = remainder.find('/');
    if (slash == std::string::npos || slash == 0 || slash == remainder.size() - 1)
    {
        return Result<ParsedReference>::fail("S3 reference must include bucket and key", StatusCode::INVALID_FORMAT);
    }

    ParsedReference parsed;
    parsed.bucket = remainder.substr(0, slash);
    parsed.key = remainder.substr(slash + 1);
    return Result<ParsedReference>::ok(parsed);
}

std::string S3Storage::makeS3Reference(const std::string& objectKey) const
{
    const std::string key = joinKey(Config::instance().s3Prefix(), objectKey);
    return "s3://" + Config::instance().s3Bucket() + "/" + key;
}

std::string S3Storage::buildBaseArgs() const
{
    std::ostringstream args;
    args << "aws";

    if (!Config::instance().s3Region().empty())
    {
        args << " --region " << shellQuote(Config::instance().s3Region());
    }

    if (!Config::instance().s3EndpointUrl().empty())
    {
        args << " --endpoint-url " << shellQuote(Config::instance().s3EndpointUrl());
    }

    return args.str();
}

std::string S3Storage::buildCopyCommand(
    const std::string& source,
    const std::string& destination) const
{
    std::ostringstream command;
    command << buildBaseArgs() << " s3 cp " << shellQuote(source) << " " << shellQuote(destination);
    return command.str();
}
