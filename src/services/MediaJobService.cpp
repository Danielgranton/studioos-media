#include "MediaJobService.hpp"

#include <chrono>
#include <mutex>

#include <nlohmann/json.hpp>

#include "core/StatusCode.hpp"

using json = nlohmann::json;

namespace
{
constexpr const char* kQueuedStatus = "QUEUED";
}

Result<MediaJobService::JobRecord> MediaJobService::submitJob(
    const std::string& assetReference,
    const std::string& operation,
    const std::string& parametersJson)
{
    if (assetReference.empty())
    {
        return Result<JobRecord>::fail("Asset reference is required", StatusCode::INVALID_FORMAT);
    }

    if (operation.empty())
    {
        return Result<JobRecord>::fail("Operation is required", StatusCode::INVALID_FORMAT);
    }

    const std::string normalizedParameters = parametersJson.empty() ? "{}" : parametersJson;
    try
    {
        const auto parsed = json::parse(normalizedParameters);
        (void)parsed;
    }
    catch (const json::exception&)
    {
        return Result<JobRecord>::fail("Parameters JSON is invalid", StatusCode::INVALID_FORMAT);
    }

    std::lock_guard<std::mutex> lock(mMutex);
    JobRecord record;
    record.jobId = nextJobId();
    record.status = kQueuedStatus;
    record.assetReference = assetReference;
    record.operation = operation;
    record.parametersJson = normalizedParameters;
    record.createdAtUnixMs = nowUnixMs();
    record.updatedAtUnixMs = record.createdAtUnixMs;

    mJobs.emplace(record.jobId, record);
    return Result<JobRecord>::ok(record);
}

Result<MediaJobService::JobRecord> MediaJobService::getJob(const std::string& jobId) const
{
    std::lock_guard<std::mutex> lock(mMutex);
    const auto it = mJobs.find(jobId);
    if (it == mJobs.end())
    {
        return Result<JobRecord>::fail("Job not found", StatusCode::FILE_NOT_FOUND);
    }

    return Result<JobRecord>::ok(it->second);
}

std::int64_t MediaJobService::nowUnixMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string MediaJobService::nextJobId()
{
    return "job-" + std::to_string(mNextJobId++);
}
