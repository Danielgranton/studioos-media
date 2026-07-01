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

int Config::imageQuality() const
{
    return mImageQuality;
}