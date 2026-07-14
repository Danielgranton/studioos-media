#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>

#include <opencv2/opencv.hpp>

#include <grpcpp/grpcpp.h>

#include "grpc/MediaServer.hpp"

namespace fs = std::filesystem;

namespace
{
void createAsset(const fs::path& path)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << "asset";
}

void createVideoAsset(const fs::path& path)
{
    const std::string command =
        "ffmpeg -y -f lavfi -i color=c=blue:s=1280x720:d=10 -f lavfi -i sine=frequency=1000:duration=10 -shortest -pix_fmt yuv420p -c:v libx264 -c:a aac \"" +
        path.string() + "\" >/dev/null 2>&1";
    if (std::system(command.c_str()) != 0)
    {
        std::cerr << "failed to create video asset" << std::endl;
        std::exit(10);
    }
}
}

int main()
{
    const fs::path tempDir = "temp/grpc-ad-service-tests";
    fs::create_directories(tempDir);

    const fs::path assetPath = tempDir / "video.mp4";
    createVideoAsset(assetPath);

    MediaServer server;
    grpc::ServerContext context;

    media::AdCreateRequest createRequest;
    createRequest.set_assetpath(assetPath.string());
    createRequest.set_title("Pre-roll");
    createRequest.set_durationseconds(12);
    createRequest.set_clickurl("https://example.com");

    media::AdResponse createResponse;
    if (!server.CreateVideoAd(&context, &createRequest, &createResponse).ok())
    {
        std::cerr << "CreateVideoAd RPC failed" << std::endl;
        return 1;
    }

    if (createResponse.adid().empty())
    {
        std::cerr << "CreateVideoAd returned an empty ad id" << std::endl;
        return 2;
    }

    const auto now = std::chrono::system_clock::now();
    media::AdScheduleRequest scheduleRequest;
    scheduleRequest.set_adid(createResponse.adid());
    scheduleRequest.set_startsatunixms(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() - 1000);
    scheduleRequest.set_endsatunixms(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() + 1000);
    scheduleRequest.set_priority(5);

    media::AdResponse scheduleResponse;
    if (!server.ScheduleAd(&context, &scheduleRequest, &scheduleResponse).ok())
    {
        std::cerr << "ScheduleAd RPC failed" << std::endl;
        return 3;
    }

    media::AdIdRequest idRequest;
    idRequest.set_adid(createResponse.adid());

    media::AdResponse impressionResponse;
    if (!server.RecordImpression(&context, &idRequest, &impressionResponse).ok())
    {
        std::cerr << "RecordImpression RPC failed" << std::endl;
        return 4;
    }

    media::AdResponse clickResponse;
    if (!server.RecordClick(&context, &idRequest, &clickResponse).ok())
    {
        std::cerr << "RecordClick RPC failed" << std::endl;
        return 5;
    }

    media::AdReportResponse reportResponse;
    if (!server.GetAdReport(&context, &idRequest, &reportResponse).ok())
    {
        std::cerr << "GetAdReport RPC failed" << std::endl;
        return 6;
    }

    if (reportResponse.adid() != createResponse.adid() || reportResponse.reportjson().empty())
    {
        std::cerr << "GetAdReport returned unexpected data" << std::endl;
        return 7;
    }

    std::cout << "grpc ad service smoke test passed" << std::endl;
    return 0;
}
