#include "StreamingService.hpp"

#include <fstream>
#include <sstream>
#include <vector>

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

struct Rendition
{
    int width;
    int height;
    int videoBitrateKbps;
    int audioBitrateKbps;
    const char* name;
};

const std::vector<Rendition> kRenditions = {
    {1920, 1080, 5000, 192, "1080p"},
    {1280, 720, 3000, 128, "720p"},
    {854, 480, 1600, 96, "480p"},
    {640, 360, 800, 64, "360p"}
};

bool generateRendition(
    const std::string& inputPath,
    const std::string& directory,
    const Rendition& rendition)
{
    const std::string playlist = directory + "/" + rendition.name + ".m3u8";
    const std::string segmentPattern = directory + "/" + rendition.name + "_%03d.ts";
    const std::string filter =
        "scale=w=" + std::to_string(rendition.width) + ":h=" + std::to_string(rendition.height) +
        ":force_original_aspect_ratio=decrease,pad=" + std::to_string(rendition.width) + ":" +
        std::to_string(rendition.height) + ":(ow-iw)/2:(oh-ih)/2";
    const std::string args =
        "-y -i \"" + inputPath + "\" -vf \"" + filter +
        "\" -c:v libx264 -preset veryfast -crf 23 -c:a aac -b:a " +
        std::to_string(rendition.audioBitrateKbps) + "k -b:v " +
        std::to_string(rendition.videoBitrateKbps) + "k -f hls -hls_time 4 -hls_playlist_type vod -hls_segment_filename \"" +
        segmentPattern + "\" \"" + playlist + "\"";
    return FFmpeg::run(args);
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
    std::ostringstream master;
    master << "#EXTM3U\n#EXT-X-VERSION:3\n";

    for (const auto& rendition : kRenditions)
    {
        if (!generateRendition(inputPath, directory, rendition))
        {
            return Result<std::string>::fail("Failed to generate adaptive bitrate stream", StatusCode::VIDEO_ERROR);
        }

        const int bandwidth = (rendition.videoBitrateKbps + rendition.audioBitrateKbps) * 1000;
        master << "#EXT-X-STREAM-INF:BANDWIDTH=" << bandwidth
               << ",RESOLUTION=" << rendition.width << "x" << rendition.height << "\n"
               << rendition.name << ".m3u8\n";
    }

    if (!writeTextFile(masterPlaylistPath, master.str()))
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
