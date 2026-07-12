#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/Result.hpp"

class MediaJobService
{
public:
    MediaJobService();

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
    Result<JobRecord> updateJob(
        const std::string& jobId,
        const std::string& status,
        const std::string& resultReference = "",
        const std::string& errorMessage = "");

    std::vector<JobRecord> recoverPendingJobs() const;

private:
    static std::int64_t nowUnixMs();

    std::string nextJobId();
    void loadFromDisk();
    void saveToDiskLocked() const;
    static bool isRecoverableStatus(const std::string& status);
    static std::size_t jobIndex(const std::string& jobId);

    mutable std::mutex mMutex;
    std::unordered_map<std::string, JobRecord> mJobs;
    std::size_t mNextJobId = 1;
    std::string mStorePath;
};
