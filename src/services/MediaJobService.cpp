#include "MediaJobService.hpp"

#include <chrono>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/StatusCode.hpp"

using json = nlohmann::json;

namespace
{
constexpr const char* kQueuedStatus = "QUEUED";
constexpr const char* kRunningStatus = "RUNNING";
constexpr const char* kDefaultStorePath = "temp/media-jobs.json";
}

MediaJobService::MediaJobService()
{
    if (const char* overridePath = std::getenv("MEDIA_JOB_STORE_PATH");
        overridePath != nullptr && overridePath[0] != '\0')
    {
        mStorePath = overridePath;
    }
    else
    {
        mStorePath = kDefaultStorePath;
    }

    loadFromDisk();
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
    saveToDiskLocked();
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

Result<MediaJobService::JobRecord> MediaJobService::updateJob(
    const std::string& jobId,
    const std::string& status,
    const std::string& resultReference,
    const std::string& errorMessage)
{
    std::lock_guard<std::mutex> lock(mMutex);
    const auto it = mJobs.find(jobId);
    if (it == mJobs.end())
    {
        return Result<JobRecord>::fail("Job not found", StatusCode::FILE_NOT_FOUND);
    }

    it->second.status = status;
    it->second.resultReference = resultReference;
    it->second.errorMessage = errorMessage;
    it->second.updatedAtUnixMs = nowUnixMs();
    saveToDiskLocked();
    return Result<JobRecord>::ok(it->second);
}

std::vector<MediaJobService::JobRecord> MediaJobService::recoverPendingJobs() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    std::vector<JobRecord> jobs;
    for (const auto& [_, job] : mJobs)
    {
        if (isRecoverableStatus(job.status))
        {
            jobs.push_back(job);
        }
    }
    return jobs;
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

bool MediaJobService::isRecoverableStatus(const std::string& status)
{
    return status == kQueuedStatus || status == kRunningStatus;
}

std::size_t MediaJobService::jobIndex(const std::string& jobId)
{
    constexpr const char* prefix = "job-";
    if (jobId.rfind(prefix, 0) != 0)
    {
        return 0;
    }

    try
    {
        return static_cast<std::size_t>(std::stoul(jobId.substr(4)));
    }
    catch (...)
    {
        return 0;
    }
}

void MediaJobService::loadFromDisk()
{
    std::lock_guard<std::mutex> lock(mMutex);
    std::ifstream file(mStorePath);
    if (!file.is_open())
    {
        return;
    }

    try
    {
        json data;
        file >> data;

        mJobs.clear();
        mNextJobId = 1;

        for (const auto& item : data.value("jobs", json::array()))
        {
            JobRecord job;
            job.jobId = item.value("jobId", "");
            job.status = item.value("status", "");
            job.assetReference = item.value("assetReference", "");
            job.operation = item.value("operation", "");
            job.parametersJson = item.value("parametersJson", "{}");
            job.resultReference = item.value("resultReference", "");
            job.errorMessage = item.value("errorMessage", "");
            job.createdAtUnixMs = item.value("createdAtUnixMs", 0LL);
            job.updatedAtUnixMs = item.value("updatedAtUnixMs", 0LL);

            if (!job.jobId.empty())
            {
                mJobs.emplace(job.jobId, job);
                mNextJobId = std::max(mNextJobId, jobIndex(job.jobId) + 1);
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to load media job store: " << e.what() << std::endl;
        mJobs.clear();
        mNextJobId = 1;
    }
}

void MediaJobService::saveToDiskLocked() const
{
    try
    {
        const std::filesystem::path storePath(mStorePath);
        if (storePath.has_parent_path())
        {
            std::filesystem::create_directories(storePath.parent_path());
        }

        json data;
        data["jobs"] = json::array();
        for (const auto& [_, job] : mJobs)
        {
            data["jobs"].push_back({
                {"jobId", job.jobId},
                {"status", job.status},
                {"assetReference", job.assetReference},
                {"operation", job.operation},
                {"parametersJson", job.parametersJson},
                {"resultReference", job.resultReference},
                {"errorMessage", job.errorMessage},
                {"createdAtUnixMs", job.createdAtUnixMs},
                {"updatedAtUnixMs", job.updatedAtUnixMs},
            });
        }

        std::ofstream file(storePath);
        if (!file.is_open())
        {
            std::cerr << "Failed to persist media job store: unable to open "
                      << mStorePath << std::endl;
            return;
        }

        file << data.dump(2);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to persist media job store: " << e.what() << std::endl;
    }
}
