#include "StreamingService.hpp"

#include <fstream>
#include <sstream>

#include "ffmpeg/FFmpeg.hpp"
#include "utils/FileUtils.hpp"
#include "utils/Logger.hpp"

namespace
{
std::string normalizeOutputDirectory(const std::string& outputDirectory)
{
    if (outputDirectory.empty())
    {
        return "temp/streaming";
    }

    return outputDirectory;
}

bool writeTextFile(const std::string& path, const std::string& content)
{
    std::ofstream stream(path, std::ios::out | std::ios::trunc);
    if (!stream)
    {
        return false;
    }

    stream << content;
    stream.close();
    return true;
}
}

Result<std::string> StreamingService::generateHls(const std::string& inputPath, const std::string& outputDirectory)
{
    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Input video not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    const std::string directory = normalizeOutputDirectory(outputDirectory);
    FileUtils::createDirectory(directory);

    const std::string playlistPath = directory + "/playlist.m3u8";
    const std::string args = "-y -i \"" + inputPath + "\" -c:v libx264 -preset veryfast -crf 23 -c:a aac -b:a 128k -f hls -hls_time 2 -hls_playlist_type vod -hls_segment_filename \"" + directory + "/segment_%03d.ts\" \"" + playlistPath + "\"";

    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to generate HLS stream", StatusCode::VIDEO_ERROR);
    }

    return Result<std::string>::ok(playlistPath);
}

Result<std::string> StreamingService::generateDash(const std::string& inputPath, const std::string& outputDirectory)
{
    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Input video not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    const std::string directory = normalizeOutputDirectory(outputDirectory);
    FileUtils::createDirectory(directory);

    const std::string manifestPath = directory + "/manifest.mpd";
    const std::string args = "-y -i \"" + inputPath + "\" -map 0:v:0 -map 0:a:0 -c:v libx264 -b:v:0 1000k -c:a aac -b:a 128k -f dash -seg_duration 2 -window_size 5 -adaptation_sets \"id=0,streams=v id=1,streams=a\" \"" + manifestPath + "\"";

    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to generate DASH stream", StatusCode::VIDEO_ERROR);
    }

    return Result<std::string>::ok(manifestPath);
}

Result<std::string> StreamingService::generateAdaptiveBitrate(const std::string& inputPath, const std::string& outputDirectory)
{
    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Input video not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    const std::string directory = normalizeOutputDirectory(outputDirectory);
    FileUtils::createDirectory(directory);

    const std::string masterPlaylistPath = directory + "/master.m3u8";
    const std::string args = "-y -i \"" + inputPath + "\" -vf scale=w=640:h=360 -c:v libx264 -preset veryfast -crf 26 -c:a aac -b:a 64k -f hls -hls_time 2 -hls_playlist_type vod -hls_segment_filename \"" + directory + "/360p_%03d.ts\" \"" + directory + "/360p.m3u8\" -vf scale=w=1280:h=720 -c:v libx264 -preset veryfast -crf 23 -c:a aac -b:a 128k -f hls -hls_time 2 -hls_playlist_type vod -hls_segment_filename \"" + directory + "/720p_%03d.ts\" \"" + directory + "/720p.m3u8\"";

    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to generate adaptive bitrate stream", StatusCode::VIDEO_ERROR);
    }

    const std::string masterContent = "#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=800000,RESOLUTION=640x360\n360p.m3u8\n#EXT-X-STREAM-INF:BANDWIDTH=2200000,RESOLUTION=1280x720\n720p.m3u8\n";
    if (!writeTextFile(masterPlaylistPath, masterContent))
    {
        return Result<std::string>::fail("Failed to create adaptive bitrate playlist", StatusCode::INTERNAL_ERROR);
    }

    return Result<std::string>::ok(masterPlaylistPath);
}

Result<std::string> StreamingService::generateSegments(const std::string& inputPath, const std::string& outputDirectory, int segmentDurationSeconds)
{
    if (inputPath.empty() || !FileUtils::exists(inputPath))
    {
        return Result<std::string>::fail("Input video not found", StatusCode::FILE_NOT_FOUND);
    }

    if (!FFmpeg::isAvailable())
    {
        return Result<std::string>::fail("FFmpeg is not available", StatusCode::VIDEO_ERROR);
    }

    const std::string directory = normalizeOutputDirectory(outputDirectory);
    FileUtils::createDirectory(directory);

    const std::string args = "-y -i \"" + inputPath + "\" -c:v libx264 -preset veryfast -crf 23 -c:a aac -f segment -segment_time " + std::to_string(segmentDurationSeconds) + " -reset_timestamps 1 -map 0 \"" + directory + "/segment_%03d.mp4\"";
    if (!FFmpeg::run(args))
    {
        return Result<std::string>::fail("Failed to generate media segments", StatusCode::VIDEO_ERROR);
    }

    return Result<std::string>::ok(directory);
}

Result<std::string> StreamingService::createPlaylist(const std::string& outputDirectory, const std::string& playlistName)
{
    const std::string directory = normalizeOutputDirectory(outputDirectory);
    FileUtils::createDirectory(directory);

    const std::string playlistPath = directory + "/" + playlistName;
    const std::string content = "#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-PLAYLIST-TYPE:VOD\n";
    if (!writeTextFile(playlistPath, content))
    {
        return Result<std::string>::fail("Failed to create playlist", StatusCode::INTERNAL_ERROR);
    }

    return Result<std::string>::ok(playlistPath);
}
