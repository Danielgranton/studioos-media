#pragma once

#include <string>
#include <vector>

#include "core/Result.hpp"

class StreamingService
{
public:
    Result<std::string> generateHls(const std::string& inputPath, const std::string& outputDirectory);

    Result<std::string> generateDash(const std::string& inputPath, const std::string& outputDirectory);

    Result<std::string> generateAdaptiveBitrate(const std::string& inputPath, const std::string& outputDirectory);

    Result<std::string> generateSegments(const std::string& inputPath, const std::string& outputDirectory, int segmentDurationSeconds = 2);

    Result<std::string> createPlaylist(const std::string& outputDirectory, const std::string& playlistName = "playlist.m3u8");
};
