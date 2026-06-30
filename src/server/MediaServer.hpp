#pragma once

#include <grpcpp/grpcpp.h>
#include "media.grpc.pb.h"

class MediaServer final : public media::MediaService::Service {

public:

    grpc::Status CompressImage(
        grpc::ServerContext* context,
        const media::ImageRequest* request,
        media::ImageResponse* response
    ) override;

};