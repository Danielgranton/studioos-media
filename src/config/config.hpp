#pragma once

#include <string>

class Config {
public:
    static Config& instance();

    bool load(const std::string& filename = "config/config.json");
    bool save(const std::string& filename = "config/config.json") const;
    bool reload(const std::string& filename = "config/config.json");

    int grpcPort() const;
    const std::string& tempFolder() const;
    const std::string& assetsFolder() const;
    const std::string& s3Bucket() const;
    const std::string& s3Region() const;
    const std::string& s3EndpointUrl() const;
    const std::string& s3Prefix() const;
    bool s3UsePathStyle() const;
    int imageQuality() const;

private:
    Config() = default;

    int mGrpcPort = 50051;
    std::string mTempFolder = "temp";
    std::string mAssetsFolder = "assets";
    std::string mS3Bucket;
    std::string mS3Region;
    std::string mS3EndpointUrl;
    std::string mS3Prefix;
    bool mS3UsePathStyle = false;
    int mImageQuality = 85;
};
