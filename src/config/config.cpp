#include "config.hpp"

#include <fstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace
{
constexpr const char* kDefaultConfigPath = "config/config.json";
}

Config& Config::instance()
{
    static Config config;
    return config;
}

bool Config::load(const std::string& filename)
{
    std::ifstream file(filename.empty() ? kDefaultConfigPath : filename);

    if (!file.is_open())
        return false;

    json j;
    file >> j;

    mGrpcPort = j.value("grpc", json::object()).value("port", mGrpcPort);
    mTempFolder = j.value("storage", json::object()).value("temp", mTempFolder);
    mAssetsFolder = j.value("storage", json::object()).value("assets", mAssetsFolder);
    mS3Bucket = j.value("storage", json::object()).value("s3Bucket", mS3Bucket);
    mS3Region = j.value("storage", json::object()).value("s3Region", mS3Region);
    mS3EndpointUrl = j.value("storage", json::object()).value("s3EndpointUrl", mS3EndpointUrl);
    mS3Prefix = j.value("storage", json::object()).value("s3Prefix", mS3Prefix);
    mS3UsePathStyle = j.value("storage", json::object()).value("s3UsePathStyle", mS3UsePathStyle);
    mImageQuality = j.value("image", json::object()).value("quality", mImageQuality);

    return true;
}

bool Config::save(const std::string& filename) const
{
    const std::string target = filename.empty() ? kDefaultConfigPath : filename;
    std::ofstream file(target);

    if (!file.is_open())
        return false;

    json j;
    j["grpc"]["port"] = mGrpcPort;
    j["storage"]["temp"] = mTempFolder;
    j["storage"]["assets"] = mAssetsFolder;
    j["storage"]["s3Bucket"] = mS3Bucket;
    j["storage"]["s3Region"] = mS3Region;
    j["storage"]["s3EndpointUrl"] = mS3EndpointUrl;
    j["storage"]["s3Prefix"] = mS3Prefix;
    j["storage"]["s3UsePathStyle"] = mS3UsePathStyle;
    j["image"]["quality"] = mImageQuality;

    file << j.dump(2);
    return true;
}

bool Config::reload(const std::string& filename)
{
    return load(filename.empty() ? kDefaultConfigPath : filename);
}

int Config::grpcPort() const
{
    return mGrpcPort;
}

const std::string& Config::tempFolder() const
{
    return mTempFolder;
}

const std::string& Config::assetsFolder() const
{
    return mAssetsFolder;
}

const std::string& Config::s3Bucket() const
{
    return mS3Bucket;
}

const std::string& Config::s3Region() const
{
    return mS3Region;
}

const std::string& Config::s3EndpointUrl() const
{
    return mS3EndpointUrl;
}

const std::string& Config::s3Prefix() const
{
    return mS3Prefix;
}

bool Config::s3UsePathStyle() const
{
    return mS3UsePathStyle;
}

int Config::imageQuality() const
{
    return mImageQuality;
}
