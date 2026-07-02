#include "AdService.hpp"

#include <algorithm>
#include <sstream>
#include <utility>
#include <limits>

#include <nlohmann/json.hpp>

#include "utils/FileUtils.hpp"

using json = nlohmann::json;

namespace
{
constexpr int kMinimumDurationSeconds = 1;
}

Result<std::string> AdService::createVideoAd(
    const std::string& assetPath,
    const std::string& title,
    int durationSeconds,
    const std::string& clickUrl)
{
    return createAd(AdType::Video, assetPath, title, durationSeconds, clickUrl);
}

Result<std::string> AdService::createImageAd(
    const std::string& assetPath,
    const std::string& title,
    int durationSeconds,
    const std::string& clickUrl)
{
    return createAd(AdType::Image, assetPath, title, durationSeconds, clickUrl);
}

Result<std::string> AdService::createAudioAd(
    const std::string& assetPath,
    const std::string& title,
    int durationSeconds,
    const std::string& clickUrl)
{
    return createAd(AdType::Audio, assetPath, title, durationSeconds, clickUrl);
}

Result<std::string> AdService::scheduleAd(
    const std::string& adId,
    const std::chrono::system_clock::time_point& startsAt,
    const std::chrono::system_clock::time_point& endsAt,
    int priority)
{
    std::lock_guard<std::mutex> lock(mMutex);

    if (!mAds.contains(adId))
    {
        return Result<std::string>::fail("Ad not found", StatusCode::FILE_NOT_FOUND);
    }

    if (endsAt <= startsAt)
    {
        return Result<std::string>::fail("Schedule end must be after start", StatusCode::INVALID_FORMAT);
    }

    mSchedules[adId] = AdSchedule{
        .adId = adId,
        .startsAt = startsAt,
        .endsAt = endsAt,
        .priority = priority
    };

    return Result<std::string>::ok(adId);
}

Result<std::string> AdService::selectAd(
    AdType type,
    const std::chrono::system_clock::time_point& at)
{
    std::lock_guard<std::mutex> lock(mMutex);

    std::string selectedId;
    int selectedPriority = std::numeric_limits<int>::min();

    for (const auto& [adId, ad] : mAds)
    {
        if (ad.type != type)
        {
            continue;
        }

        const auto scheduleIt = mSchedules.find(adId);
        if (scheduleIt == mSchedules.end() || !isActive(scheduleIt->second, at))
        {
            continue;
        }

        if (scheduleIt->second.priority > selectedPriority)
        {
            selectedPriority = scheduleIt->second.priority;
            selectedId = adId;
        }
    }

    if (selectedId.empty())
    {
        return Result<std::string>::fail("No active ad found", StatusCode::FILE_NOT_FOUND);
    }

    return Result<std::string>::ok(selectedId);
}

Result<std::string> AdService::recordImpression(const std::string& adId)
{
    std::lock_guard<std::mutex> lock(mMutex);

    auto it = mAds.find(adId);
    if (it == mAds.end())
    {
        return Result<std::string>::fail("Ad not found", StatusCode::FILE_NOT_FOUND);
    }

    ++mMetrics[adId].impressions;
    return Result<std::string>::ok(adId);
}

Result<std::string> AdService::recordClick(const std::string& adId)
{
    std::lock_guard<std::mutex> lock(mMutex);

    auto it = mAds.find(adId);
    if (it == mAds.end())
    {
        return Result<std::string>::fail("Ad not found", StatusCode::FILE_NOT_FOUND);
    }

    ++mMetrics[adId].clicks;
    return Result<std::string>::ok(adId);
}

Result<std::string> AdService::report(const std::string& adId) const
{
    std::lock_guard<std::mutex> lock(mMutex);

    auto adIt = mAds.find(adId);
    if (adIt == mAds.end())
    {
        return Result<std::string>::fail("Ad not found", StatusCode::FILE_NOT_FOUND);
    }

    const auto metricsIt = mMetrics.find(adId);
    const AdMetrics metrics = metricsIt == mMetrics.end() ? AdMetrics{} : metricsIt->second;
    const auto scheduleIt = mSchedules.find(adId);

    json payload;
    payload["id"] = adIt->second.id;
    payload["type"] = typeToString(adIt->second.type);
    payload["title"] = adIt->second.title;
    payload["assetPath"] = adIt->second.assetPath;
    payload["clickUrl"] = adIt->second.clickUrl;
    payload["durationSeconds"] = adIt->second.durationSeconds;
    payload["impressions"] = metrics.impressions;
    payload["clicks"] = metrics.clicks;
    payload["scheduled"] = scheduleIt != mSchedules.end();
    payload["active"] = scheduleIt != mSchedules.end() && isActive(scheduleIt->second, std::chrono::system_clock::now());

    return Result<std::string>::ok(payload.dump(2));
}

Result<std::string> AdService::createAd(
    AdType type,
    const std::string& assetPath,
    const std::string& title,
    int durationSeconds,
    const std::string& clickUrl)
{
    if (assetPath.empty() || !FileUtils::exists(assetPath))
    {
        return Result<std::string>::fail("Ad asset not found", StatusCode::FILE_NOT_FOUND);
    }

    if (durationSeconds < kMinimumDurationSeconds)
    {
        return Result<std::string>::fail("Ad duration must be at least one second", StatusCode::INVALID_FORMAT);
    }

    std::lock_guard<std::mutex> lock(mMutex);
    const std::string id = nextId();

    mAds.emplace(id, AdCreative{
        .id = id,
        .type = type,
        .assetPath = assetPath,
        .title = title,
        .clickUrl = clickUrl,
        .durationSeconds = durationSeconds
    });

    mMetrics.emplace(id, AdMetrics{});

    return Result<std::string>::ok(id);
}

std::string AdService::typeToString(AdType type)
{
    switch (type)
    {
        case AdType::Video:
            return "video";
        case AdType::Image:
            return "image";
        case AdType::Audio:
            return "audio";
    }

    return "unknown";
}

bool AdService::isActive(
    const AdSchedule& schedule,
    const std::chrono::system_clock::time_point& at)
{
    return at >= schedule.startsAt && at <= schedule.endsAt;
}

std::string AdService::nextId()
{
    return "ad-" + std::to_string(mNextId++);
}
