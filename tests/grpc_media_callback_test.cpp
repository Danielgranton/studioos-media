#include <condition_variable>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "grpc/MediaServer.hpp"

namespace
{
class CallbackCaptureService final : public media::MediaCallbackService::Service
{
public:
    explicit CallbackCaptureService(std::mutex& mutex,
                                    std::condition_variable& cv,
                                    media::MediaJobCallbackRequest& request,
                                    bool& received)
        : mMutex(mutex), mCv(cv), mRequest(request), mReceived(received)
    {
    }

    grpc::Status ReportMediaJob(
        grpc::ServerContext*,
        const media::MediaJobCallbackRequest* request,
        media::MediaJobCallbackResponse* response) override
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mRequest = *request;
            mReceived = true;
        }

        response->set_accepted(true);
        mCv.notify_one();
        return grpc::Status::OK;
    }

private:
    std::mutex& mMutex;
    std::condition_variable& mCv;
    media::MediaJobCallbackRequest& mRequest;
    bool& mReceived;
};
}

int main()
{
    std::mutex mutex;
    std::condition_variable cv;
    bool callbackReceived = false;
    media::MediaJobCallbackRequest receivedRequest;

    CallbackCaptureService callbackService(mutex, cv, receivedRequest, callbackReceived);

    grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:18080", grpc::InsecureServerCredentials());
    builder.RegisterService(&callbackService);

    std::unique_ptr<grpc::Server> callbackServer = builder.BuildAndStart();
    if (!callbackServer)
    {
        std::cerr << "callback gRPC server failed to start" << std::endl;
        return 1;
    }

    std::thread serverThread([&callbackServer]() {
        callbackServer->Wait();
    });

    setenv("MEDIA_CALLBACK_GRPC_HOST", "127.0.0.1", 1);
    setenv("MEDIA_CALLBACK_GRPC_PORT", "18080", 1);

    MediaServer server;
    grpc::ServerContext context;

    media::MediaJobRequest submitRequest;
    submitRequest.set_assetreference("/tmp/studioos-missing-input.wav");
    submitRequest.set_operation("audio.normalize");
    submitRequest.set_parametersjson("{}");

    media::MediaJobResponse submitResponse;
    if (!server.SubmitMediaJob(&context, &submitRequest, &submitResponse).ok())
    {
        std::cerr << "SubmitMediaJob RPC failed" << std::endl;
        callbackServer->Shutdown();
        serverThread.join();
        return 2;
    }

    std::unique_lock<std::mutex> lock(mutex);
    const bool signalled = cv.wait_for(lock, std::chrono::seconds(5), [&] {
        return callbackReceived;
    });
    lock.unlock();

    callbackServer->Shutdown();
    serverThread.join();

    if (!signalled)
    {
        std::cerr << "callback was not received" << std::endl;
        return 3;
    }

    if (receivedRequest.jobid() != submitResponse.jobid())
    {
        std::cerr << "callback job id mismatch" << std::endl;
        return 4;
    }

    if (receivedRequest.status() != "FAILED")
    {
        std::cerr << "callback payload did not mark the job failed" << std::endl;
        return 5;
    }

    if (receivedRequest.resultreference() != "")
    {
        std::cerr << "failed callback should not carry a result reference" << std::endl;
        return 6;
    }

    std::cout << "grpc media callback smoke test passed" << std::endl;
    return 0;
}
