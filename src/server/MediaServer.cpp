#include "MediaServer.hpp"

grpc::Status MediaServer::CompressImage(
    grpc::ServerContext*,
    const media::ImageRequest* request,
    media::ImageResponse* response)
{

    response->set_outputpath("compressed_" + request->imagepath());

    return grpc::Status::OK;
}