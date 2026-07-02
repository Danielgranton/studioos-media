#include <filesystem>
#include <iostream>

#include <grpcpp/grpcpp.h>

#include "grpc/MediaServer.hpp"

int main()
{
    MediaServer server;
    grpc::ServerContext context;

    media::MediaJobRequest submitRequest;
    submitRequest.set_assetreference("s3://studioos-media/channels/song.mp3");
    submitRequest.set_operation("compress_audio");
    submitRequest.set_parametersjson(R"({"format":"mp3"})");

    media::MediaJobResponse submitResponse;
    if (!server.SubmitMediaJob(&context, &submitRequest, &submitResponse).ok())
    {
        std::cerr << "SubmitMediaJob RPC failed" << std::endl;
        return 1;
    }

    if (submitResponse.jobid().empty() || submitResponse.status() != "QUEUED")
    {
        std::cerr << "SubmitMediaJob returned unexpected data" << std::endl;
        return 2;
    }

    media::MediaJobLookupRequest lookupRequest;
    lookupRequest.set_jobid(submitResponse.jobid());

    media::MediaJobResponse lookupResponse;
    if (!server.GetMediaJob(&context, &lookupRequest, &lookupResponse).ok())
    {
        std::cerr << "GetMediaJob RPC failed" << std::endl;
        return 3;
    }

    if (lookupResponse.jobid() != submitResponse.jobid() || lookupResponse.assetreference() != submitRequest.assetreference())
    {
        std::cerr << "GetMediaJob returned unexpected data" << std::endl;
        return 4;
    }

    std::cout << "grpc media job smoke test passed" << std::endl;
    return 0;
}
