#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#include "core/Result.hpp"

class MediaJobService
{
public:
    struct JobRecord
    {
        std::string jobId;
        std::string status;
        std::string assetReference;
        std::string operation;
        std::string parametersJson;
        std::string resultReference;
        std::string errorMessage;
        std::int64_t createdAtUnixMs = 0;
        std::int64_t updatedAtUnixMs = 0;
    };

    Result<JobRecord> submitJob(
        const std::string& assetReference,
        const std::string& operation,
        const std::string& parametersJson = "{}");

    Result<JobRecord> getJob(const std::string& jobId) const;

private:
    static std::int64_t nowUnixMs();

    std::string nextJobId();

    mutable std::mutex mMutex;
    std::unordered_map<std::string, JobRecord> mJobs;
    std::size_t mNextJobId = 1;
};
