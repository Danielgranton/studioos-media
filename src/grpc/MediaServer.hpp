#pragma once

#include <grpcpp/grpcpp.h>

#include "media.grpc.pb.h"

#include "../services/ImageService.hpp"
#include "../services/VideoService.hpp"

class MediaServer final : public media::MediaService::Service
{
private:
    ImageService imageService;
    VideoService videoService;

public:
    grpc::Status Health(
        grpc::ServerContext*,
        const media::HealthRequest*,
        media::HealthResponse*) override;

    grpc::Status CompressImage(
        grpc::ServerContext*,
        const media::ImageRequest*,
        media::ImageResponse*) override;

    grpc::Status CompressVideo(
        grpc::ServerContext*,
        const media::VideoRequest*,
        media::VideoResponse*) override;

    grpc::Status GenerateThumbnail(
        grpc::ServerContext*,
        const media::VideoRequest*,
        media::ThumbnailResponse*) override;

    grpc::Status ExtractAudio(
        grpc::ServerContext*,
        const media::VideoRequest*,
        media::AudioResponse*) override;
};