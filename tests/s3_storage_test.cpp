#include <filesystem>
#include <fstream>
#include <iostream>

#include "config/config.hpp"
#include "storage/S3Storage.hpp"

namespace fs = std::filesystem;

int main()
{
    auto parsed = S3Storage::parseReference("s3://studioos-media/uploads/song.wav");
    if (!parsed.success || parsed.value.bucket != "studioos-media" || parsed.value.key != "uploads/song.wav")
    {
        std::cerr << "failed to parse valid S3 reference" << std::endl;
        return 1;
    }

    auto invalid = S3Storage::parseReference("studioos-media/uploads/song.wav");
    if (invalid.success)
    {
        std::cerr << "accepted invalid S3 reference" << std::endl;
        return 2;
    }

    const fs::path tempDir = "temp/s3-storage-tests";
    fs::create_directories(tempDir);
    const fs::path configPath = tempDir / "config.json";

    {
        std::ofstream stream(configPath);
        stream << R"({
  "grpc": { "port": 50051 },
  "storage": {
    "temp": "temp",
    "assets": "assets",
    "s3Bucket": "studioos-media",
    "s3Region": "us-east-1",
    "s3Prefix": "processed"
  },
  "image": { "quality": 85 }
})";
    }

    if (!Config::instance().reload(configPath.string()))
    {
        std::cerr << "failed to load test config" << std::endl;
        return 3;
    }

    S3Storage storage;
    if (!storage.configured())
    {
        std::cerr << "S3 storage should be configured" << std::endl;
        return 4;
    }

    if (storage.makeS3Reference("artist/song.mp3") != "s3://studioos-media/processed/artist/song.mp3")
    {
        std::cerr << "unexpected generated S3 reference" << std::endl;
        return 5;
    }

    std::cout << "s3 storage smoke test passed" << std::endl;
    return 0;
}
