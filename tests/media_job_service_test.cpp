#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "services/MediaJobService.hpp"

int main()
{
    const std::filesystem::path storePath = "temp/test-media-jobs.json";
    std::filesystem::remove(storePath);
    setenv("MEDIA_JOB_STORE_PATH", storePath.c_str(), 1);

    MediaJobService service;

    auto submitResult = service.submitJob(
        "s3://studioos-media/uploads/artist-track.wav",
        "normalize_audio",
        R"({"targetLufs":-16})");

    if (!submitResult.success)
    {
        std::cerr << "job submission failed: " << submitResult.message << std::endl;
        return 1;
    }

    if (submitResult.value.jobId.empty() || submitResult.value.status != "QUEUED")
    {
        std::cerr << "unexpected submission response" << std::endl;
        return 2;
    }

    auto getResult = service.getJob(submitResult.value.jobId);
    if (!getResult.success || getResult.value.assetReference != "s3://studioos-media/uploads/artist-track.wav")
    {
        std::cerr << "job retrieval failed: " << getResult.message << std::endl;
        return 3;
    }

    MediaJobService restartedService;
    auto recoveredJobs = restartedService.recoverPendingJobs();
    if (recoveredJobs.empty() || recoveredJobs.front().jobId != submitResult.value.jobId)
    {
        std::cerr << "recovered job store did not contain the queued job" << std::endl;
        return 4;
    }

    std::cout << "media job service smoke test passed" << std::endl;
    return 0;
}
