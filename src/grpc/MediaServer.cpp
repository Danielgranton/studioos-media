#include "MediaServer.hpp"

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