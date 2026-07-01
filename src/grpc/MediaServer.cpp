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
    response->set_outputpath(
        imageService.compress(request->imagepath())
    );

    return grpc::Status::OK;
}

grpc::Status MediaServer::CompressVideo(
    grpc::ServerContext*,
    const media::VideoRequest* request,
    media::VideoResponse* response)
{
    response->set_outputpath(
        videoService.compress(request->videopath())
    );

    return grpc::Status::OK;
}

grpc::Status MediaServer::GenerateThumbnail(
    grpc::ServerContext*,
    const media::VideoRequest* request,
    media::ThumbnailResponse* response)
{
    response->set_thumbnailpath(
        videoService.thumbnail(request->videopath())
    );

    return grpc::Status::OK;
}

grpc::Status MediaServer::ExtractAudio(
    grpc::ServerContext*,
    const media::VideoRequest* request,
    media::AudioResponse* response)
{
    response->set_audiopath(
        videoService.extractAudio(request->videopath())
    );

    return grpc::Status::OK;
}