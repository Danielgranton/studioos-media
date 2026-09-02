#include <iostream>
#include <cstdlib>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include "grpc/MediaServer.hpp"
#include "config/config.hpp"
#include "utils/FileUtils.hpp"
#include "utils/Logger.hpp"

using grpc::Server;
using grpc::ServerBuilder;

namespace {
std::string readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}
}

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

    const std::string bindHost = Config::instance().grpcHost();
    const bool loopback = bindHost == "127.0.0.1" || bindHost == "::1" || bindHost == "localhost";
    const char* allowNonLoopback = std::getenv("STUDIOOS_MEDIA_ALLOW_NON_LOOPBACK");
    const bool explicitlyAllowed = allowNonLoopback != nullptr
            && std::string(allowNonLoopback) == "true";
    if (!loopback && !explicitlyAllowed)
    {
        std::cerr << "Refusing non-loopback media bind at " << bindHost
                  << ". Set STUDIOOS_MEDIA_ALLOW_NON_LOOPBACK=true only on a private network.\n";
        return 1;
    }

    MediaServer service;

    ServerBuilder builder;

    std::shared_ptr<grpc::ServerCredentials> credentials;
    if (Config::instance().grpcTlsEnabled())
    {
        const auto& certificateFile = Config::instance().grpcTlsCertificateFile();
        const auto& keyFile = Config::instance().grpcTlsKeyFile();
        const auto& caFile = Config::instance().grpcTlsCaFile();
        const std::string certificate = readFile(certificateFile);
        const std::string key = readFile(keyFile);
        const std::string ca = readFile(caFile);
        if (certificate.empty() || key.empty()
                || (Config::instance().grpcTlsRequireClientCertificate() && ca.empty()))
        {
            std::cerr << "TLS is enabled but certificate, key, or CA files are missing\n";
            return 1;
        }

        grpc::SslServerCredentialsOptions options;
        options.pem_key_cert_pairs.push_back({key, certificate});
        options.pem_root_certs = ca;
        options.client_certificate_request = Config::instance().grpcTlsRequireClientCertificate()
                ? GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY
                : GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE;
        credentials = grpc::SslServerCredentials(options);
    }
    else
    {
        credentials = grpc::InsecureServerCredentials();
    }

    builder.AddListeningPort(
        bindHost + ":" + std::to_string(Config::instance().grpcPort()), credentials);

    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout << "StudioOS Media Service listening on "
              << bindHost << ":"
              << Config::instance().grpcPort() << "\n";

    server->Wait();

    Logger::info("Service started using temp directory: " + tempFolder);

    return 0;
}
