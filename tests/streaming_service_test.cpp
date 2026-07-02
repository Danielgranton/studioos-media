#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "services/StreamingService.hpp"

namespace fs = std::filesystem;

int main()
{
    const fs::path tempDir = "temp/streaming-tests";
    fs::create_directories(tempDir);

    const fs::path inputPath = tempDir / "input.mp4";
    const std::string generateCommand =
        "ffmpeg -y -f lavfi -i testsrc2=size=160x90:rate=8 -f lavfi -i sine=frequency=1000:duration=2 -shortest -c:v libx264 -pix_fmt yuv420p -c:a aac " +
        inputPath.string() + " >/dev/null 2>&1";

    if (std::system(generateCommand.c_str()) != 0)
    {
        std::cerr << "failed to generate sample video" << std::endl;
        return 1;
    }

    StreamingService service;

    const fs::path hlsDir = tempDir / "hls";
    auto hlsResult = service.generateHls(inputPath.string(), hlsDir.string());
    if (!hlsResult.success || !fs::exists(hlsDir / "playlist.m3u8"))
    {
        std::cerr << "HLS generation failed: " << hlsResult.message << std::endl;
        return 2;
    }

    const fs::path dashDir = tempDir / "dash";
    auto dashResult = service.generateDash(inputPath.string(), dashDir.string());
    if (!dashResult.success || !fs::exists(dashDir / "manifest.mpd"))
    {
        std::cerr << "DASH generation failed: " << dashResult.message << std::endl;
        return 3;
    }

    const fs::path abrDir = tempDir / "abr";
    auto abrResult = service.generateAdaptiveBitrate(inputPath.string(), abrDir.string());
    if (!abrResult.success || !fs::exists(abrDir / "master.m3u8"))
    {
        std::cerr << "adaptive bitrate generation failed: " << abrResult.message << std::endl;
        return 4;
    }

    std::cout << "streaming service smoke test passed" << std::endl;
    return 0;
}
