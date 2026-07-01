#include <iostream>
#include <grpcpp/grpcpp.h>
#include "grpc/MediaServer.hpp"
#include "config/Config.hpp"

using grpc::Server;
using grpc::ServerBuilder;

int main() {

    if(!Config::instance().load("config/config.json"))
    {
        return 1;
    }

    MediaServer service;

    ServerBuilder builder;

    builder.AddListeningPort(
        "0.0.0.0:50051",
        grpc::InsecureServerCredentials());

    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout << "StudioOS Media Service running on port 50051\n";

    server->Wait();

    return 0;
}