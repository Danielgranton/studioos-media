#include <iostream>
#include <grpcpp/grpcpp.h>
#include "grpc/MediaServer.hpp"
#include "config/config.hpp"
#include "utils/FileUtils.hpp"
#include "utils/Logger.hpp"

using grpc::Server;
using grpc::ServerBuilder;

int main() {

    if (!Config::instance().load("config/config.json"))
    {
        Config::instance().save("config/config.json");
    }

    const std::string tempFolder = Config::instance().tempFolder();
    if (!FileUtils::exists(tempFolder))
    {
        FileUtils::createDirectory(tempFolder);
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

    Logger::info("Service started using temp directory: " + tempFolder);

    return 0;
}