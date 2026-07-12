#include "MediaServer.hpp"

#include <chrono>
#include <vector>

#include "core/Result.hpp"

namespace
{
grpc::StatusCode toGrpcStatus(StatusCode status)
{
    switch (status)
    {
        case StatusCode::FILE_NOT_FOUND:
            return grpc::StatusCode::NOT_FOUND;
        case StatusCode::INVALID_FORMAT:
            return grpc::StatusCode::INVALID_ARGUMENT;
        case StatusCode::INTERNAL_ERROR:
        case StatusCode::IMAGE_ERROR:
        case StatusCode::VIDEO_ERROR:
        case StatusCode::AUDIO_ERROR:
        default:
            return grpc::StatusCode::INTERNAL;
    }
}

template<typename T>
grpc::Status makeError(const Result<T>& result, const std::string& fallback)
{
    return grpc::Status(
        toGrpcStatus(result.status),
        result.message.empty() ? fallback : result.message);
}

std::chrono::system_clock::time_point fromUnixMs(long long value)
{
    return std::chrono::system_clock::time_point(std::chrono::milliseconds(value));
}
}

MediaServer::MediaServer()
    : jobDispatcher(jobService, imageService, videoService, audioService)
{
    for (const auto& job : jobService.recoverPendingJobs())
    {
        jobDispatcher.enqueue(job);
    }
}

grpc::Status MediaServer::Health(
    grpc::ServerContext*,
    const media::HealthRequest*,
    media::HealthResponse* response)
{
    response->set_status("UP");
    return grpc::Status::OK;
}

grpc::Status MediaServer::CompressImage(
    grpc::ServerContext*,
    const media::ImageRequest* request,
    media::ImageResponse* response)
{
    auto result = imageService.compress(request->imagepath());

    if (!result.success)
    {
        return grpc::Status(
            grpc::StatusCode::NOT_FOUND,
            result.message.empty() ? "Image compression failed" : result.message);
    }

    response->set_outputpath(result.value);

    return grpc::Status::OK;
}

grpc::Status MediaServer::CompressVideo(
    grpc::ServerContext*,
    const media::VideoRequest* request,
    media::VideoResponse* response)
{
    auto result = videoService.compress(request->videopath());

    if (!result.success)
    {
        return grpc::Status(
            grpc::StatusCode::NOT_FOUND,
            result.message.empty() ? "Video compression failed" : result.message);
    }

    response->set_outputpath(result.value);

    return grpc::Status::OK;
}

grpc::Status MediaServer::GenerateThumbnail(
    grpc::ServerContext*,
    const media::VideoRequest* request,
    media::ThumbnailResponse* response)
{
    auto result = videoService.thumbnail(request->videopath());

    if (!result.success)
    {
        return grpc::Status(
            grpc::StatusCode::NOT_FOUND,
            result.message.empty() ? "Thumbnail generation failed" : result.message);
    }

    response->set_thumbnailpath(result.value);

    return grpc::Status::OK;
}

grpc::Status MediaServer::ExtractAudio(
    grpc::ServerContext*,
    const media::VideoRequest* request,
    media::AudioResponse* response)
{
    auto result = videoService.extractAudio(request->videopath());

    if (!result.success)
    {
        return grpc::Status(
            grpc::StatusCode::NOT_FOUND,
            result.message.empty() ? "Audio extraction failed" : result.message);
    }

    response->set_audiopath(result.value);

    return grpc::Status::OK;
}

grpc::Status MediaServer::ConvertVideo(
    grpc::ServerContext*,
    const media::VideoConvertRequest* request,
    media::VideoResponse* response)
{
    auto result = videoService.convert(request->videopath(), request->format());

    if (!result.success)
    {
        return grpc::Status(
            grpc::StatusCode::NOT_FOUND,
            result.message.empty() ? "Video conversion failed" : result.message);
    }

    response->set_outputpath(result.value);

    return grpc::Status::OK;
}

grpc::Status MediaServer::TrimVideo(
    grpc::ServerContext*,
    const media::VideoTrimRequest* request,
    media::VideoResponse* response)
{
    auto result = videoService.trim(request->videopath(), request->start(), request->end());

    if (!result.success)
    {
        return grpc::Status(
            grpc::StatusCode::NOT_FOUND,
            result.message.empty() ? "Video trimming failed" : result.message);
    }

    response->set_outputpath(result.value);

    return grpc::Status::OK;
}

grpc::Status MediaServer::MergeVideos(
    grpc::ServerContext*,
    const media::VideoMergeRequest* request,
    media::VideoResponse* response)
{
    std::vector<std::string> paths(request->videopaths().begin(), request->videopaths().end());
    auto result = videoService.merge(paths, request->outputpath());

    if (!result.success)
    {
        return grpc::Status(
            grpc::StatusCode::NOT_FOUND,
            result.message.empty() ? "Video merge failed" : result.message);
    }

    response->set_outputpath(result.value);

    return grpc::Status::OK;
}

grpc::Status MediaServer::EncodeAudio(
    grpc::ServerContext*,
    const media::AudioFormatRequest* request,
    media::AudioResponse* response)
{
    auto result = audioService.encode(request->inputpath(), request->format());

    if (!result.success)
    {
        return grpc::Status(
            grpc::StatusCode::NOT_FOUND,
            result.message.empty() ? "Audio encoding failed" : result.message);
    }

    response->set_audiopath(result.value);

    return grpc::Status::OK;
}

grpc::Status MediaServer::NormalizeAudio(
    grpc::ServerContext*,
    const media::AudioRequest* request,
    media::AudioResponse* response)
{
    auto result = audioService.normalize(request->inputpath());

    if (!result.success)
    {
        return grpc::Status(
            grpc::StatusCode::NOT_FOUND,
            result.message.empty() ? "Audio normalization failed" : result.message);
    }

    response->set_audiopath(result.value);

    return grpc::Status::OK;
}

grpc::Status MediaServer::DenoiseAudio(
    grpc::ServerContext*,
    const media::AudioRequest* request,
    media::AudioResponse* response)
{
    auto result = audioService.denoise(request->inputpath());

    if (!result.success)
    {
        return grpc::Status(
            grpc::StatusCode::NOT_FOUND,
            result.message.empty() ? "Audio denoising failed" : result.message);
    }

    response->set_audiopath(result.value);

    return grpc::Status::OK;
}

grpc::Status MediaServer::MergeAudio(
    grpc::ServerContext*,
    const media::AudioMergeRequest* request,
    media::AudioResponse* response)
{
    std::vector<std::string> paths(request->inputpaths().begin(), request->inputpaths().end());
    auto result = audioService.merge(paths, request->outputpath());

    if (!result.success)
    {
        return grpc::Status(
            grpc::StatusCode::NOT_FOUND,
            result.message.empty() ? "Audio merge failed" : result.message);
    }

    response->set_audiopath(result.value);

    return grpc::Status::OK;
}

grpc::Status MediaServer::TrimAudio(
    grpc::ServerContext*,
    const media::AudioTrimRequest* request,
    media::AudioResponse* response)
{
    auto result = audioService.trim(request->inputpath(), request->start(), request->end());

    if (!result.success)
    {
        return grpc::Status(
            grpc::StatusCode::NOT_FOUND,
            result.message.empty() ? "Audio trimming failed" : result.message);
    }

    response->set_audiopath(result.value);

    return grpc::Status::OK;
}

grpc::Status MediaServer::ConvertAudio(
    grpc::ServerContext*,
    const media::AudioFormatRequest* request,
    media::AudioResponse* response)
{
    auto result = audioService.convert(request->inputpath(), request->format());

    if (!result.success)
    {
        return grpc::Status(
            grpc::StatusCode::NOT_FOUND,
            result.message.empty() ? "Audio conversion failed" : result.message);
    }

    response->set_audiopath(result.value);

    return grpc::Status::OK;
}

grpc::Status MediaServer::SubmitMediaJob(
    grpc::ServerContext*,
    const media::MediaJobRequest* request,
    media::MediaJobResponse* response)
{
    auto result = jobService.submitJob(
        request->assetreference(),
        request->operation(),
        request->parametersjson());

    if (!result.success)
    {
        return makeError(result, "Media job submission failed");
    }

    const auto& job = result.value;
    jobDispatcher.enqueue(job);
    response->set_jobid(job.jobId);
    response->set_status(job.status);
    response->set_assetreference(job.assetReference);
    response->set_operation(job.operation);
    response->set_parametersjson(job.parametersJson);
    response->set_resultreference(job.resultReference);
    response->set_errormessage(job.errorMessage);
    response->set_createdatunixms(job.createdAtUnixMs);
    response->set_updatedatunixms(job.updatedAtUnixMs);
    return grpc::Status::OK;
}

grpc::Status MediaServer::GetMediaJob(
    grpc::ServerContext*,
    const media::MediaJobLookupRequest* request,
    media::MediaJobResponse* response)
{
    auto result = jobService.getJob(request->jobid());
    if (!result.success)
    {
        return makeError(result, "Media job not found");
    }

    const auto& job = result.value;
    response->set_jobid(job.jobId);
    response->set_status(job.status);
    response->set_assetreference(job.assetReference);
    response->set_operation(job.operation);
    response->set_parametersjson(job.parametersJson);
    response->set_resultreference(job.resultReference);
    response->set_errormessage(job.errorMessage);
    response->set_createdatunixms(job.createdAtUnixMs);
    response->set_updatedatunixms(job.updatedAtUnixMs);
    return grpc::Status::OK;
}

grpc::Status MediaServer::CreateVideoAd(
    grpc::ServerContext*,
    const media::AdCreateRequest* request,
    media::AdResponse* response)
{
    auto result = adService.createVideoAd(
        request->assetpath(),
        request->title(),
        request->durationseconds(),
        request->clickurl());

    if (!result.success)
    {
        return makeError(result, "Video ad creation failed");
    }

    response->set_adid(result.value);
    response->set_status("created");
    return grpc::Status::OK;
}

grpc::Status MediaServer::CreateImageAd(
    grpc::ServerContext*,
    const media::AdCreateRequest* request,
    media::AdResponse* response)
{
    auto result = adService.createImageAd(
        request->assetpath(),
        request->title(),
        request->durationseconds(),
        request->clickurl());

    if (!result.success)
    {
        return makeError(result, "Image ad creation failed");
    }

    response->set_adid(result.value);
    response->set_status("created");
    return grpc::Status::OK;
}

grpc::Status MediaServer::CreateAudioAd(
    grpc::ServerContext*,
    const media::AdCreateRequest* request,
    media::AdResponse* response)
{
    auto result = adService.createAudioAd(
        request->assetpath(),
        request->title(),
        request->durationseconds(),
        request->clickurl());

    if (!result.success)
    {
        return makeError(result, "Audio ad creation failed");
    }

    response->set_adid(result.value);
    response->set_status("created");
    return grpc::Status::OK;
}

grpc::Status MediaServer::ScheduleAd(
    grpc::ServerContext*,
    const media::AdScheduleRequest* request,
    media::AdResponse* response)
{
    auto result = adService.scheduleAd(
        request->adid(),
        fromUnixMs(request->startsatunixms()),
        fromUnixMs(request->endsatunixms()),
        request->priority());

    if (!result.success)
    {
        return makeError(result, "Ad scheduling failed");
    }

    response->set_adid(result.value);
    response->set_status("scheduled");
    return grpc::Status::OK;
}

grpc::Status MediaServer::RecordImpression(
    grpc::ServerContext*,
    const media::AdIdRequest* request,
    media::AdResponse* response)
{
    auto result = adService.recordImpression(request->adid());
    if (!result.success)
    {
        return makeError(result, "Impression tracking failed");
    }

    response->set_adid(result.value);
    response->set_status("impression_recorded");
    return grpc::Status::OK;
}

grpc::Status MediaServer::RecordClick(
    grpc::ServerContext*,
    const media::AdIdRequest* request,
    media::AdResponse* response)
{
    auto result = adService.recordClick(request->adid());
    if (!result.success)
    {
        return makeError(result, "Click tracking failed");
    }

    response->set_adid(result.value);
    response->set_status("click_recorded");
    return grpc::Status::OK;
}

grpc::Status MediaServer::GetAdReport(
    grpc::ServerContext*,
    const media::AdIdRequest* request,
    media::AdReportResponse* response)
{
    auto result = adService.report(request->adid());
    if (!result.success)
    {
        return makeError(result, "Ad report failed");
    }

    response->set_adid(request->adid());
    response->set_reportjson(result.value);
    return grpc::Status::OK;
}
