#include "Config.hpp"

#include <fstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

Config& Config::instance()
{
    static Config config;
    return config;
}

bool Config::load(const std::string& filename)
{
    std::ifstream file(filename);

    if (!file.is_open())
        return false;

    json j;
    file >> j;

    mGrpcPort = j["grpc"]["port"];
    mTempFolder = j["storage"]["temp"];
    mAssetsFolder = j["storage"]["assets"];
    mImageQuality = j["image"]["quality"];

    return true;
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