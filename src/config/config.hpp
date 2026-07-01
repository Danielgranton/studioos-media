#pragma once

#include <string>

class Config {
public:
    static Config& instance();

    bool load(const std::string& filename);

    int grpcPort() const;

    const std::string& tempFolder() const;

    const std::string& assetsFolder() const;

    int imageQuality() const;

private:
    Config() = default;

    int mGrpcPort = 50051;
    std::string mTempFolder = "temp";
    std::string mAssetsFolder = "assets";
    int mImageQuality = 85;
};