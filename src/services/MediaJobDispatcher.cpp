#include "MediaJobDispatcher.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <thread>

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
    const Result<std::string>& result)
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

    media::MediaJobCallbackResponse response;
    grpc::ClientContext context;
    grpc::Status status = stub->ReportMediaJob(&context, request, &response);
    if (!status.ok() || !response.accepted())
    {
        Logger::warning(
            "Media job callback gRPC failed for " + job.jobId + ": " + status.error_message());
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
    AudioService& audioService)
{
    ImageProcessor imageProcessor;
    const json params = parseParameters(job.parametersJson);
    const std::string tempFolder = Config::instance().tempFolder();
    FileUtils::createDirectory(tempFolder);

    S3Storage storage;
    std::string localInputPath = job.assetReference;

    if (S3Storage::isS3Reference(job.assetReference))
    {
        const auto parsedReference = S3Storage::parseReference(job.assetReference);
        if (!parsedReference.success)
        {
            return Result<std::string>::fail(parsedReference.message, parsedReference.status);
        }

        localInputPath = tempFolder + "/input_" + job.jobId + "_" + FileUtils::filename(parsedReference.value.key);
        const auto download = storage.downloadFile(job.assetReference, localInputPath);
        if (!download.success)
        {
            return Result<std::string>::fail(download.message, download.status);
        }
    }
    else if (!FileUtils::exists(localInputPath))
    {
        return Result<std::string>::fail("Asset reference not found", StatusCode::FILE_NOT_FOUND);
    }

    Result<std::string> processingResult = Result<std::string>::fail("Unsupported operation", StatusCode::INVALID_FORMAT);

    if (containsOperation(job.operation, "audio.normalize"))
    {
        processingResult = audioService.normalize(localInputPath);
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

    std::string resultReference = processingResult.value;
    if (storage.configured())
    {
        const std::string objectKey = "processed/" + job.jobId + "/" + FileUtils::filename(processingResult.value);
        const auto upload = storage.uploadFile(processingResult.value, objectKey);
        if (!upload.success)
        {
            return Result<std::string>::fail(upload.message, upload.status);
        }
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
      mWorker(&MediaJobDispatcher::workerLoop, this)
{
}

MediaJobDispatcher::~MediaJobDispatcher()
{
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mStop = true;
    }
    mCv.notify_all();
    if (mWorker.joinable())
    {
        mWorker.join();
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

void MediaJobDispatcher::processJob(const MediaJobService::JobRecord& job)
{
    constexpr auto kProcessingDelay = std::chrono::milliseconds(50);
    std::this_thread::sleep_for(kProcessingDelay);

    (void)mJobService.updateJob(job.jobId, "RUNNING");

    const auto result = processMediaJob(job, mImageService, mVideoService, mAudioService);
    if (result.success)
    {
        (void)mJobService.updateJob(job.jobId, "SUCCESS", result.value, "");
    }
    else
    {
        (void)mJobService.updateJob(job.jobId, "FAILED", "", result.message);
    }

    reportCallbackGrpc(job, result);
}
