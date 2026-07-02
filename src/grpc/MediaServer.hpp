#pragma once

#include <grpcpp/grpcpp.h>

#include "media.grpc.pb.h"

#include "../services/AudioService.hpp"
#include "../services/AdService.hpp"
#include "../services/MediaJobService.hpp"
#include "../services/ImageService.hpp"
#include "../services/VideoService.hpp"

class MediaServer final : public media::MediaService::Service
{
private:
    ImageService imageService;
    VideoService videoService;
    AudioService audioService;
    AdService adService;
    MediaJobService jobService;

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

    grpc::Status ConvertVideo(
        grpc::ServerContext*,
        const media::VideoConvertRequest*,
        media::VideoResponse*) override;

    grpc::Status TrimVideo(
        grpc::ServerContext*,
        const media::VideoTrimRequest*,
        media::VideoResponse*) override;

    grpc::Status MergeVideos(
        grpc::ServerContext*,
        const media::VideoMergeRequest*,
        media::VideoResponse*) override;

    grpc::Status EncodeAudio(
        grpc::ServerContext*,
        const media::AudioFormatRequest*,
        media::AudioResponse*) override;

    grpc::Status NormalizeAudio(
        grpc::ServerContext*,
        const media::AudioRequest*,
        media::AudioResponse*) override;

    grpc::Status DenoiseAudio(
        grpc::ServerContext*,
        const media::AudioRequest*,
        media::AudioResponse*) override;

    grpc::Status MergeAudio(
        grpc::ServerContext*,
        const media::AudioMergeRequest*,
        media::AudioResponse*) override;

    grpc::Status TrimAudio(
        grpc::ServerContext*,
        const media::AudioTrimRequest*,
        media::AudioResponse*) override;

    grpc::Status ConvertAudio(
        grpc::ServerContext*,
        const media::AudioFormatRequest*,
        media::AudioResponse*) override;

    grpc::Status SubmitMediaJob(
        grpc::ServerContext*,
        const media::MediaJobRequest*,
        media::MediaJobResponse*) override;

    grpc::Status GetMediaJob(
        grpc::ServerContext*,
        const media::MediaJobLookupRequest*,
        media::MediaJobResponse*) override;

    grpc::Status CreateVideoAd(
        grpc::ServerContext*,
        const media::AdCreateRequest*,
        media::AdResponse*) override;

    grpc::Status CreateImageAd(
        grpc::ServerContext*,
        const media::AdCreateRequest*,
        media::AdResponse*) override;

    grpc::Status CreateAudioAd(
        grpc::ServerContext*,
        const media::AdCreateRequest*,
        media::AdResponse*) override;

    grpc::Status ScheduleAd(
        grpc::ServerContext*,
        const media::AdScheduleRequest*,
        media::AdResponse*) override;

    grpc::Status RecordImpression(
        grpc::ServerContext*,
        const media::AdIdRequest*,
        media::AdResponse*) override;

    grpc::Status RecordClick(
        grpc::ServerContext*,
        const media::AdIdRequest*,
        media::AdResponse*) override;

    grpc::Status GetAdReport(
        grpc::ServerContext*,
        const media::AdIdRequest*,
        media::AdReportResponse*) override;
};
