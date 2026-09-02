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
    mGrpcHost = j.value("grpc", json::object()).value("host", mGrpcHost);
    const auto tls = j.value("grpc", json::object()).value("tls", json::object());
    mGrpcTlsEnabled = tls.value("enabled", mGrpcTlsEnabled);
    mGrpcTlsRequireClientCertificate = tls.value("requireClientCertificate", mGrpcTlsRequireClientCertificate);
    mGrpcTlsCertificateFile = tls.value("certificateFile", mGrpcTlsCertificateFile);
    mGrpcTlsKeyFile = tls.value("keyFile", mGrpcTlsKeyFile);
    mGrpcTlsCaFile = tls.value("caFile", mGrpcTlsCaFile);
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
    j["grpc"]["host"] = mGrpcHost;
    j["grpc"]["tls"]["enabled"] = mGrpcTlsEnabled;
    j["grpc"]["tls"]["requireClientCertificate"] = mGrpcTlsRequireClientCertificate;
    j["grpc"]["tls"]["certificateFile"] = mGrpcTlsCertificateFile;
    j["grpc"]["tls"]["keyFile"] = mGrpcTlsKeyFile;
    j["grpc"]["tls"]["caFile"] = mGrpcTlsCaFile;
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

const std::string& Config::grpcHost() const
{
    return mGrpcHost;
}

bool Config::grpcTlsEnabled() const { return mGrpcTlsEnabled; }
bool Config::grpcTlsRequireClientCertificate() const { return mGrpcTlsRequireClientCertificate; }
const std::string& Config::grpcTlsCertificateFile() const { return mGrpcTlsCertificateFile; }
const std::string& Config::grpcTlsKeyFile() const { return mGrpcTlsKeyFile; }
const std::string& Config::grpcTlsCaFile() const { return mGrpcTlsCaFile; }

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
