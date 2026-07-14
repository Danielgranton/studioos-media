#include "AdService.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <utility>
#include <limits>

#include <nlohmann/json.hpp>

#include "config/config.hpp"
#include "ffmpeg/CommandRunner.hpp"
#include "media/ImageProcessor.hpp"
#include "services/StreamingService.hpp"
#include "utils/FileUtils.hpp"

using json = nlohmann::json;

namespace
{
constexpr int kMinimumDurationSeconds = 1;
constexpr int kVideoMinDurationSeconds = 5;
constexpr int kVideoMaxDurationSeconds = 30;
constexpr int kVideoMinWidth = 320;
constexpr int kVideoMinHeight = 180;
constexpr std::size_t kVideoMaxFileSizeBytes = 250ULL * 1024ULL * 1024ULL;
constexpr int kImageMinWidth = 300;
constexpr int kImageMinHeight = 300;
constexpr std::size_t kImageMaxFileSizeBytes = 50ULL * 1024ULL * 1024ULL;

std::string adWorkDirectory(const std::string& assetPath)
{
    const std::string folder = Config::instance().tempFolder() + "/ads/" + FileUtils::stem(assetPath);
    FileUtils::createDirectory(Config::instance().tempFolder() + "/ads");
    FileUtils::createDirectory(folder);
    return folder;
}

std::string buildJson(const AdService::ProcessedAd& processed)
{
    json payload;
    payload["originalAsset"] = processed.originalAsset;
    payload["normalizedVideo"] = processed.normalizedVideo;
    payload["variant1080"] = processed.variant1080;
    payload["variant720"] = processed.variant720;
    payload["variant480"] = processed.variant480;
    payload["variant360"] = processed.variant360;
    payload["hlsManifest"] = processed.hlsManifest;
    payload["dashManifest"] = processed.dashManifest;
    payload["thumbnail"] = processed.thumbnail;
    payload["poster"] = processed.poster;
    payload["preview"] = processed.preview;
    payload["metadata"] = processed.metadataJson.empty() ? json::object() : json::parse(processed.metadataJson, nullptr, false);
    payload["checksum"] = processed.checksum;
    payload["qualityScore"] = processed.qualityScore;
    payload["valid"] = processed.valid;
    payload["warnings"] = processed.warnings;
    return payload.dump(2);
}

std::string computeSha256(const std::string& path)
{
    std::string output;
    if (!CommandRunner::run("sha256sum \"" + path + "\"", &output))
    {
        return {};
    }

    const auto space = output.find(' ');
    if (space == std::string::npos)
    {
        return {};
    }

    return output.substr(0, space);
}

json videoMetadataJson(const VideoMetadata& metadata)
{
    return json{
        {"width", metadata.width},
        {"height", metadata.height},
        {"codec", metadata.codec},
        {"fps", metadata.fps},
        {"durationSeconds", metadata.durationSeconds},
        {"bitrate", metadata.bitrate},
        {"sizeBytes", metadata.sizeBytes},
        {"audioCodec", metadata.audioCodec},
        {"audioChannels", metadata.audioChannels},
        {"audioSampleRate", metadata.audioSampleRate}
    };
}

double scoreVideo(const VideoMetadata& metadata)
{
    double score = 100.0;

    if (metadata.width < 1280)
    {
        score -= 10.0;
    }
    if (metadata.height < 720)
    {
        score -= 10.0;
    }
    if (metadata.bitrate > 0 && metadata.bitrate < 1'500'000)
    {
        score -= 10.0;
    }
    if (metadata.audioCodec != "aac" && !metadata.audioCodec.empty())
    {
        score -= 5.0;
    }
    if (metadata.fps < 24.0 || metadata.fps > 60.0)
    {
        score -= 5.0;
    }

    return std::max(0.0, score);
}

double scoreImage(int width, int height, std::size_t fileSize)
{
    double score = 100.0;
    if (width < 1200)
    {
        score -= 10.0;
    }
    if (height < 628)
    {
        score -= 10.0;
    }
    if (fileSize > 10ULL * 1024ULL * 1024ULL)
    {
        score -= 5.0;
    }

    return std::max(0.0, score);
}

void addWarning(AdService::ProcessedAd& processed, const std::string& warning)
{
    processed.warnings.push_back(warning);
}

std::string firstOrEmpty(const std::string& value)
{
    return value;
}
}

Result<std::string> AdService::createVideoAd(
    const std::string& assetPath,
    const std::string& title,
    int durationSeconds,
    const std::string& clickUrl)
{
    auto processed = processVideoAd(assetPath);
    if (!processed.success || !processed.value.valid)
    {
        return Result<std::string>::fail(
            processed.message.empty() ? "Video ad processing failed" : processed.message,
            processed.status);
    }

    auto created = createAd(AdType::Video, assetPath, title, durationSeconds, clickUrl);
    if (!created.success)
    {
        return created;
    }

    std::lock_guard<std::mutex> lock(mMutex);
    mAds[created.value].processed = processed.value;
    return created;
}

Result<std::string> AdService::createImageAd(
    const std::string& assetPath,
    const std::string& title,
    int durationSeconds,
    const std::string& clickUrl)
{
    auto processed = processImageAd(assetPath);
    if (!processed.success || !processed.value.valid)
    {
        return Result<std::string>::fail(
            processed.message.empty() ? "Image ad processing failed" : processed.message,
            processed.status);
    }

    auto created = createAd(AdType::Image, assetPath, title, durationSeconds, clickUrl);
    if (!created.success)
    {
        return created;
    }

    std::lock_guard<std::mutex> lock(mMutex);
    mAds[created.value].processed = processed.value;
    return created;
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
    if (adIt->second.processed.valid)
    {
        payload["processed"] = json::parse(buildJson(adIt->second.processed));
    }

    return Result<std::string>::ok(payload.dump(2));
}

Result<AdService::ProcessedAd> AdService::validateVideoAd(const std::string& assetPath)
{
    ProcessedAd processed;
    processed.originalAsset = assetPath;

    if (assetPath.empty() || !FileUtils::exists(assetPath))
    {
        return Result<ProcessedAd>::fail("Ad asset not found", StatusCode::FILE_NOT_FOUND);
    }

    if (FileUtils::fileSize(assetPath) > kVideoMaxFileSizeBytes)
    {
        return Result<ProcessedAd>::fail("Video ad exceeds maximum size", StatusCode::INVALID_FORMAT);
    }

    VideoProcessor videoProcessor;
    const auto metadataResult = videoProcessor.metadata(assetPath);
    if (!metadataResult.success)
    {
        return Result<ProcessedAd>::fail(metadataResult.message, metadataResult.status);
    }

    const auto& metadata = metadataResult.value;
    if (metadata.durationSeconds < kVideoMinDurationSeconds || metadata.durationSeconds > kVideoMaxDurationSeconds)
    {
        return Result<ProcessedAd>::fail("Video ad duration must be between 5 and 30 seconds", StatusCode::INVALID_FORMAT);
    }

    if (metadata.width < kVideoMinWidth || metadata.height < kVideoMinHeight)
    {
        return Result<ProcessedAd>::fail("Video ad resolution is too small", StatusCode::INVALID_FORMAT);
    }

    if (metadata.codec != "h264" && metadata.codec != "libx264" && !metadata.codec.empty())
    {
        addWarning(processed, "Expected H264 video codec");
    }

    if (metadata.audioCodec != "aac" && !metadata.audioCodec.empty())
    {
        addWarning(processed, "Expected AAC audio codec");
    }

    if (metadata.audioCodec.empty())
    {
        return Result<ProcessedAd>::fail("Video ad must contain audio", StatusCode::INVALID_FORMAT);
    }

    if (metadata.fps <= 0.0)
    {
        return Result<ProcessedAd>::fail("Video ad frame rate could not be determined", StatusCode::VIDEO_ERROR);
    }

    processed.metadataJson = videoMetadataJson(metadata).dump(2);
    processed.checksum = computeSha256(assetPath);
    if (processed.checksum.empty())
    {
        return Result<ProcessedAd>::fail("Unable to compute video checksum", StatusCode::VIDEO_ERROR);
    }

    processed.qualityScore = scoreVideo(metadata);
    processed.valid = true;
    return Result<ProcessedAd>::ok(processed);
}

Result<AdService::ProcessedAd> AdService::processVideoAd(const std::string& assetPath)
{
    auto validated = validateVideoAd(assetPath);
    if (!validated.success)
    {
        return validated;
    }

    ProcessedAd processed = validated.value;
    const std::string root = adWorkDirectory(assetPath);

    VideoProcessor videoProcessor;
    StreamingService streamingService;

    const auto normalizeResult = videoProcessor.normalizeAudio(assetPath);
    if (normalizeResult.success)
    {
        processed.normalizedVideo = normalizeResult.value;
    }

    const auto variant1080 = videoProcessor.resize(assetPath, 1920, 1080);
    if (variant1080.success) processed.variant1080 = variant1080.value;
    const auto variant720 = videoProcessor.resize(assetPath, 1280, 720);
    if (variant720.success) processed.variant720 = variant720.value;
    const auto variant480 = videoProcessor.resize(assetPath, 854, 480);
    if (variant480.success) processed.variant480 = variant480.value;
    const auto variant360 = videoProcessor.resize(assetPath, 640, 360);
    if (variant360.success) processed.variant360 = variant360.value;

    const auto thumbnails = videoProcessor.thumbnails(assetPath, 3);
    if (thumbnails.success)
    {
        const std::string thumbDir = thumbnails.value;
        const std::string firstThumb = thumbDir + "/thumbnail_1.jpg";
        const std::string secondThumb = thumbDir + "/thumbnail_2.jpg";
        const std::string thirdThumb = thumbDir + "/thumbnail_3.jpg";
        if (FileUtils::exists(firstThumb)) processed.thumbnail = firstThumb;
        if (FileUtils::exists(secondThumb)) processed.poster = secondThumb;
        if (FileUtils::exists(thirdThumb)) addWarning(processed, "Generated additional thumbnail variant");
    }

    const auto poster = videoProcessor.extractFrame(assetPath, "0.5");
    if (poster.success)
    {
        processed.poster = poster.value;
    }

    const auto preview = videoProcessor.previewClip(assetPath, 5);
    if (preview.success)
    {
        processed.preview = preview.value;
    }

    const auto hls = videoProcessor.adaptiveStreaming(assetPath, root + "/hls");
    if (hls.success)
    {
        processed.hlsManifest = hls.value;
    }

    const auto dash = streamingService.generateDash(assetPath, root + "/dash");
    if (dash.success)
    {
        processed.dashManifest = dash.value;
    }

    processed.valid = true;
    return Result<ProcessedAd>::ok(processed);
}

Result<AdService::ProcessedAd> AdService::validateImageAd(const std::string& assetPath)
{
    ProcessedAd processed;
    processed.originalAsset = assetPath;

    if (assetPath.empty() || !FileUtils::exists(assetPath))
    {
        return Result<ProcessedAd>::fail("Ad asset not found", StatusCode::FILE_NOT_FOUND);
    }

    if (FileUtils::fileSize(assetPath) > kImageMaxFileSizeBytes)
    {
        return Result<ProcessedAd>::fail("Image ad exceeds maximum size", StatusCode::INVALID_FORMAT);
    }

    cv::Mat image = cv::imread(assetPath);
    if (image.empty())
    {
        return Result<ProcessedAd>::fail("Unable to read image ad", StatusCode::INVALID_FORMAT);
    }

    if (image.cols < kImageMinWidth || image.rows < kImageMinHeight)
    {
        return Result<ProcessedAd>::fail("Image ad resolution is too small", StatusCode::INVALID_FORMAT);
    }

    json metadata;
    metadata["width"] = image.cols;
    metadata["height"] = image.rows;
    metadata["sizeBytes"] = FileUtils::fileSize(assetPath);
    metadata["aspectRatio"] = static_cast<double>(image.cols) / static_cast<double>(image.rows);
    processed.metadataJson = metadata.dump(2);
    processed.checksum = computeSha256(assetPath);
    if (processed.checksum.empty())
    {
        return Result<ProcessedAd>::fail("Unable to compute image checksum", StatusCode::VIDEO_ERROR);
    }

    processed.qualityScore = scoreImage(image.cols, image.rows, FileUtils::fileSize(assetPath));
    processed.valid = true;
    return Result<ProcessedAd>::ok(processed);
}

Result<AdService::ProcessedAd> AdService::processImageAd(const std::string& assetPath)
{
    auto validated = validateImageAd(assetPath);
    if (!validated.success)
    {
        return validated;
    }

    ProcessedAd processed = validated.value;
    const std::string root = adWorkDirectory(assetPath);

    ImageProcessor imageProcessor;
    const auto variant1080 = imageProcessor.resize(assetPath, 1920, 1080);
    if (variant1080.success) processed.variant1080 = variant1080.value;
    const auto variant720 = imageProcessor.resize(assetPath, 1280, 720);
    if (variant720.success) processed.variant720 = variant720.value;
    const auto variant480 = imageProcessor.resize(assetPath, 854, 480);
    if (variant480.success) processed.variant480 = variant480.value;
    const auto variant360 = imageProcessor.resize(assetPath, 640, 360);
    if (variant360.success) processed.variant360 = variant360.value;

    const auto thumbnail = imageProcessor.thumbnail(assetPath, 320);
    if (thumbnail.success)
    {
        processed.thumbnail = thumbnail.value;
    }

    const auto poster = imageProcessor.squareThumbnail(assetPath, 600);
    if (poster.success)
    {
        processed.poster = poster.value;
    }

    const auto preview = imageProcessor.saveAsWebp(assetPath, 80);
    if (preview.success)
    {
        processed.preview = preview.value;
    }

    processed.valid = true;
    addWarning(processed, "Image ads do not use HLS or DASH packaging");
    addWarning(processed, "Blacklist and safety scanners are still hooks only");
    (void)root;
    return Result<ProcessedAd>::ok(processed);
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
