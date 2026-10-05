#include "MediaJobDispatcher.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <grpcpp/grpcpp.h>
#include <nlohmann/json.hpp>

#include "config/config.hpp"
#include "core/Result.hpp"
#include "core/StatusCode.hpp"
#include "media/ImageProcessor.hpp"
#include "media.grpc.pb.h"
#include "storage/S3Storage.hpp"
#include "utils/FileUtils.hpp"
#include "utils/Logger.hpp"

using json = nlohmann::json;

namespace
{
std::shared_ptr<std::mutex> sourceMutexFor(const std::string& reference)
{
    static std::mutex mutex;
    static std::unordered_map<std::string, std::weak_ptr<std::mutex>> sourceMutexes;

    std::lock_guard lock(mutex);
    if (sourceMutexes.size() > 256)
    {
        for (auto iterator = sourceMutexes.begin(); iterator != sourceMutexes.end();)
        {
            if (iterator->second.expired())
            {
                iterator = sourceMutexes.erase(iterator);
            }
            else
            {
                ++iterator;
            }
        }
    }
    auto& weakMutex = sourceMutexes[reference];
    auto sourceMutex = weakMutex.lock();
    if (!sourceMutex)
    {
        sourceMutex = std::make_shared<std::mutex>();
        weakMutex = sourceMutex;
    }
    return sourceMutex;
}

bool containsOperation(const std::string& operation, const std::string& needle)
{
    auto lhs = operation;
    auto rhs = needle;
    std::transform(lhs.begin(), lhs.end(), lhs.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    std::transform(rhs.begin(), rhs.end(), rhs.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return lhs.find(rhs) != std::string::npos;
}

std::string getEnvOrDefault(const char* name, const std::string& fallback)
{
    if (const char* value = std::getenv(name); value != nullptr && value[0] != '\0')
    {
        return value;
    }
    return fallback;
}

std::size_t workerCount()
{
    constexpr std::size_t defaultWorkers = 8;
    if (const char* configured = std::getenv("MEDIA_WORKER_COUNT");
        configured != nullptr && configured[0] != '\0')
    {
        try
        {
            return std::clamp<std::size_t>(std::stoul(configured), 1, 16);
        }
        catch (...)
        {
            Logger::warning("Invalid MEDIA_WORKER_COUNT; using the default worker count");
        }
    }

    const auto hardwareThreads = std::thread::hardware_concurrency();
    return std::max<std::size_t>(
        1,
        std::min<std::size_t>(
            defaultWorkers,
            hardwareThreads == 0 ? defaultWorkers : hardwareThreads));
}

std::string callbackTarget()
{
    if (const char* target = std::getenv("MEDIA_CALLBACK_GRPC_TARGET"); target != nullptr && target[0] != '\0')
    {
        return target;
    }

    const std::string host = getEnvOrDefault("MEDIA_CALLBACK_GRPC_HOST", "localhost");
    const std::string port = getEnvOrDefault("MEDIA_CALLBACK_GRPC_PORT", "50052");
    return host + ":" + port;
}

void reportCallbackGrpc(
    const MediaJobService::JobRecord& job,
    const Result<std::string>& result,
    int durationSeconds)
{
    constexpr int maxAttempts = 5;
    for (int attempt = 1; attempt <= maxAttempts; ++attempt)
    {
        auto channel = grpc::CreateChannel(callbackTarget(), grpc::InsecureChannelCredentials());
        auto stub = media::MediaCallbackService::NewStub(channel);

        media::MediaJobCallbackRequest request;
        request.set_jobid(job.jobId);
        request.set_externaljobid(job.jobId);
        request.set_status(result.success ? "SUCCESS" : "FAILED");
        if (result.success)
        {
            request.set_resultreference(result.value);
        }
        else
        {
            request.set_errormessage(result.message);
        }
        if (durationSeconds > 0)
        {
            request.set_durationseconds(durationSeconds);
        }

        media::MediaJobCallbackResponse response;
        grpc::ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
        grpc::Status status = stub->ReportMediaJob(&context, request, &response);
        if (status.ok() && response.accepted())
        {
            return;
        }

        Logger::warning("Media job callback gRPC failed for " + job.jobId
                + " attempt " + std::to_string(attempt) + ": " + status.error_message());
        if (attempt < maxAttempts)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(250 * attempt));
        }
    }
}

void reportProgressCallbackGrpc(const MediaJobService::JobRecord& job, int progressPercent)
{
    auto channel = grpc::CreateChannel(callbackTarget(), grpc::InsecureChannelCredentials());
    auto stub = media::MediaCallbackService::NewStub(channel);
    media::MediaJobCallbackRequest request;
    request.set_jobid(job.jobId);
    request.set_externaljobid(job.jobId);
    request.set_status("RUNNING");
    request.set_progresspercent(progressPercent);
    media::MediaJobCallbackResponse response;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(1));
    const grpc::Status status = stub->ReportMediaJob(&context, request, &response);
    if (!status.ok())
    {
        Logger::debug("Media progress callback failed for " + job.jobId + ": " + status.error_message());
    }
}

json parseParameters(const std::string& parametersJson)
{
    if (parametersJson.empty())
    {
        return json::object();
    }

    try
    {
        return json::parse(parametersJson);
    }
    catch (const json::exception&)
    {
        return json::object();
    }
}

Result<std::string> processMediaJob(
    const MediaJobService::JobRecord& job,
    ImageService& imageService,
    VideoService& videoService,
    AudioService& audioService,
    int* durationSeconds,
    const std::function<void(int)>& onProgress)
{
    using Clock = std::chrono::steady_clock;
    ImageProcessor imageProcessor;
    const json params = parseParameters(job.parametersJson);
    const std::string tempFolder = Config::instance().tempFolder();
    FileUtils::createDirectory(tempFolder);

    S3Storage storage;
    std::string localInputPath = job.assetReference;
    const auto transferStartedAt = Clock::now();

    if (S3Storage::isS3Reference(job.assetReference))
    {
        const auto parsedReference = S3Storage::parseReference(job.assetReference);
        if (!parsedReference.success)
        {
            return Result<std::string>::fail(parsedReference.message, parsedReference.status);
        }

        static const auto processCacheId = std::chrono::steady_clock::now().time_since_epoch().count();
        localInputPath = tempFolder + "/shared_input_" + std::to_string(processCacheId) + "_"
            + std::to_string(std::hash<std::string>{}(job.assetReference))
            + "_" + FileUtils::filename(parsedReference.value.key);
        const auto sourceMutex = sourceMutexFor(job.assetReference);
        std::lock_guard sourceLock(*sourceMutex);

        const bool cacheHit = FileUtils::exists(localInputPath);
        if (cacheHit)
        {
            Logger::info("Media job " + job.jobId + " inputCacheHit=true");
        }
        else
        {
            const auto download = storage.downloadFile(job.assetReference, localInputPath);
            if (!download.success)
            {
                std::error_code error;
                std::filesystem::remove(localInputPath, error);
                return Result<std::string>::fail(download.message, download.status);
            }
            Logger::info("Media job " + job.jobId + " inputDownloadMs="
                + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - transferStartedAt).count()));
        }
    }
    else if (!FileUtils::exists(localInputPath))
    {
        return Result<std::string>::fail("Asset reference not found", StatusCode::FILE_NOT_FOUND);
    }

    Result<std::string> processingResult = Result<std::string>::fail("Unsupported operation", StatusCode::INVALID_FORMAT);
    const auto processingStartedAt = Clock::now();

    if (containsOperation(job.operation, "audio.normalize"))
    {
        const auto metadata = videoService.metadata(localInputPath);
        if (metadata.success)
        {
            try
            {
                const auto payload = json::parse(metadata.value);
                *durationSeconds = static_cast<int>(std::lround(payload.value("durationSeconds", 0.0)));
            }
            catch (const json::exception&)
            {
                *durationSeconds = 0;
            }
        }
        processingResult = audioService.normalize(localInputPath, *durationSeconds, onProgress);
    }
    else if (containsOperation(job.operation, "audio.generatePreview"))
    {
        const std::string start = params.value("start", std::string("00:00:00"));
        const std::string end = params.value("end", std::string("00:00:30"));
        const int previewSeconds = params.value("durationSeconds", 70);
        processingResult = audioService.trim(localInputPath, start, end, previewSeconds, onProgress);
    }
    else if (containsOperation(job.operation, "audio.generateWaveform"))
    {
        processingResult = audioService.waveform(localInputPath);
    }
    else if (containsOperation(job.operation, "audio.compress"))
    {
        const std::string format = params.value("format", std::string("mp3"));
        processingResult = audioService.encode(localInputPath, format);
    }
    else if (containsOperation(job.operation, "video.compress"))
    {
        processingResult = videoService.compress(localInputPath);
    }
    else if (containsOperation(job.operation, "video.resize"))
    {
        const int width = params.value("width", 1920);
        const int height = params.value("height", 1080);
        processingResult = videoService.resize(localInputPath, width, height);
    }
    else if (containsOperation(job.operation, "video.watermark"))
    {
        const std::string watermarkPath = params.value("watermarkPath", std::string());
        const int x = params.value("x", 24);
        const int y = params.value("y", 24);
        processingResult = videoService.watermark(localInputPath, watermarkPath, x, y);
    }
    else if (containsOperation(job.operation, "video.rotate"))
    {
        const int degrees = params.value("degrees", 0);
        processingResult = videoService.rotate(localInputPath, degrees);
    }
    else if (containsOperation(job.operation, "video.normalizeaudio"))
    {
        processingResult = videoService.normalizeAudio(localInputPath);
    }
    else if (containsOperation(job.operation, "video.generatethumbnails"))
    {
        const int count = params.value("count", 4);
        processingResult = videoService.thumbnails(localInputPath, count);
    }
    else if (containsOperation(job.operation, "video.gifpreview"))
    {
        const int durationSeconds = params.value("durationSeconds", 5);
        processingResult = videoService.gifPreview(localInputPath, durationSeconds);
    }
    else if (containsOperation(job.operation, "video.previewclip"))
    {
        const int durationSeconds = params.value("durationSeconds", 10);
        processingResult = videoService.previewClip(localInputPath, durationSeconds);
    }
    else if (containsOperation(job.operation, "video.adaptivestreaming"))
    {
        const std::string outputDirectory = params.value("outputDirectory", std::string());
        processingResult = videoService.adaptiveStreaming(localInputPath, outputDirectory);
    }
    else if (containsOperation(job.operation, "video.metadata"))
    {
        processingResult = videoService.metadata(localInputPath);
    }
    else if (containsOperation(job.operation, "video.extractframe"))
    {
        const std::string timestamp = params.value("timestamp", std::string("00:00:00"));
        processingResult = videoService.extractFrame(localInputPath, timestamp);
    }
    else if (containsOperation(job.operation, "video.generatethumbnail") || containsOperation(job.operation, "video.thumbnail"))
    {
        processingResult = videoService.thumbnail(localInputPath);
    }
    else if (containsOperation(job.operation, "image.resize"))
    {
        const int width = params.value("width", 1080);
        const int height = params.value("height", 1080);
        processingResult = imageProcessor.resize(localInputPath, width, height);
    }
    else if (containsOperation(job.operation, "image.generatethumbnail") || containsOperation(job.operation, "image.thumbnail"))
    {
        const int size = params.value("size", 300);
        processingResult = imageProcessor.thumbnail(localInputPath, size);
    }
    else if (containsOperation(job.operation, "image.convertwebp") || containsOperation(job.operation, "image.webp"))
    {
        const int quality = params.value("quality", Config::instance().imageQuality());
        processingResult = imageProcessor.saveAsWebp(localInputPath, quality);
    }
    else if (containsOperation(job.operation, "image.compress"))
    {
        const int quality = params.value("quality", Config::instance().imageQuality());
        processingResult = imageProcessor.compress(localInputPath, quality);
    }

    if (!processingResult.success)
    {
        return Result<std::string>::fail(processingResult.message, processingResult.status);
    }
    Logger::info("Media job " + job.jobId + " transformMs="
        + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - processingStartedAt).count()));

    std::string resultReference = processingResult.value;
    if (storage.configured())
    {
        const auto uploadStartedAt = Clock::now();
        const std::string objectKey = "processed/" + job.jobId + "/" + FileUtils::filename(processingResult.value);
        const auto upload = storage.uploadFile(processingResult.value, objectKey);
        if (!upload.success)
        {
            return Result<std::string>::fail(upload.message, upload.status);
        }
        Logger::info("Media job " + job.jobId + " outputUploadMs="
            + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - uploadStartedAt).count()));
        resultReference = upload.value;
    }

    return Result<std::string>::ok(resultReference);
}
}

MediaJobDispatcher::MediaJobDispatcher(
    MediaJobService& jobService,
    ImageService& imageService,
    VideoService& videoService,
    AudioService& audioService)
    : mJobService(jobService),
      mImageService(imageService),
      mVideoService(videoService),
      mAudioService(audioService),
      mWorkers()
{
    const auto count = workerCount();
    mWorkers.reserve(count);
    for (std::size_t index = 0; index < count; ++index)
    {
        mWorkers.emplace_back(&MediaJobDispatcher::workerLoop, this);
    }
    mProgressCallbackWorker = std::thread(&MediaJobDispatcher::progressCallbackLoop, this);
    Logger::info("Media job dispatcher started with " + std::to_string(count) + " workers");
}

MediaJobDispatcher::~MediaJobDispatcher()
{
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mStop = true;
    }
    mCv.notify_all();
    for (auto& worker : mWorkers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
    {
        std::lock_guard<std::mutex> lock(mProgressMutex);
        mStopProgressCallbacks = true;
    }
    mProgressCv.notify_all();
    if (mProgressCallbackWorker.joinable())
    {
        mProgressCallbackWorker.join();
    }
}

void MediaJobDispatcher::enqueue(const MediaJobService::JobRecord& job)
{
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mQueue.push(job);
    }
    mCv.notify_one();
}

void MediaJobDispatcher::workerLoop()
{
    while (true)
    {
        MediaJobService::JobRecord job;
        {
            std::unique_lock<std::mutex> lock(mMutex);
            mCv.wait(lock, [this] {
                return mStop || !mQueue.empty();
            });

            if (mStop && mQueue.empty())
            {
                return;
            }

            job = mQueue.front();
            mQueue.pop();
        }

        processJob(job);
    }
}

void MediaJobDispatcher::enqueueProgressCallback(
    const MediaJobService::JobRecord& job,
    int progressPercent)
{
    {
        std::lock_guard<std::mutex> lock(mProgressMutex);
        mPendingProgressCallbacks[job.jobId] = {job, progressPercent};
    }
    mProgressCv.notify_one();
}

void MediaJobDispatcher::progressCallbackLoop()
{
    while (true)
    {
        std::pair<MediaJobService::JobRecord, int> update;
        {
            std::unique_lock<std::mutex> lock(mProgressMutex);
            mProgressCv.wait(lock, [this] {
                return mStopProgressCallbacks || !mPendingProgressCallbacks.empty();
            });
            if (mStopProgressCallbacks && mPendingProgressCallbacks.empty())
            {
                return;
            }
            const auto next = mPendingProgressCallbacks.begin();
            update = std::move(next->second);
            mPendingProgressCallbacks.erase(next);
        }

        reportProgressCallbackGrpc(update.first, update.second);
    }
}

void MediaJobDispatcher::processJob(const MediaJobService::JobRecord& job)
{
    const auto startedAt = std::chrono::steady_clock::now();
    const auto queuedAt = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(job.createdAtUnixMs));
    const auto queueWaitMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - queuedAt).count();

    (void)mJobService.updateJob(job.jobId, "RUNNING");
    enqueueProgressCallback(job, 0);

    int durationSeconds = 0;
    auto lastProgressCallbackAt = std::chrono::steady_clock::now();
    int lastReportedProgress = 0;
    const auto onProgress = [this, &job, &lastProgressCallbackAt, &lastReportedProgress](int progress) {
        const auto now = std::chrono::steady_clock::now();
        if (progress < 100 && progress - lastReportedProgress < 5
            && now - lastProgressCallbackAt < std::chrono::seconds(1))
        {
            return;
        }
        const int boundedProgress = std::clamp(progress, 0, 100);
        (void)mJobService.updateJob(job.jobId, "RUNNING", "", "", boundedProgress);
        lastReportedProgress = boundedProgress;
        lastProgressCallbackAt = now;
        enqueueProgressCallback(job, boundedProgress);
    };
    const auto result = processMediaJob(
        job, mImageService, mVideoService, mAudioService, &durationSeconds, onProgress);
    if (result.success)
    {
        (void)mJobService.updateJob(job.jobId, "SUCCESS", result.value, "");
    }
    else
    {
        (void)mJobService.updateJob(job.jobId, "FAILED", "", result.message);
    }

    const auto callbackStartedAt = std::chrono::steady_clock::now();
    reportCallbackGrpc(job, result, durationSeconds);
    const auto callbackFinishedAt = std::chrono::steady_clock::now();
    Logger::info("Media job " + job.jobId + " callbackMs="
        + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(callbackFinishedAt - callbackStartedAt).count()));

    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startedAt).count();
    Logger::info("Media job " + job.jobId + " " + (result.success ? "completed" : "failed")
        + " queueWaitMs=" + std::to_string(queueWaitMs)
        + " totalMs=" + std::to_string(elapsedMs));
}
