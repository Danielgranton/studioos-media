#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "AudioService.hpp"
#include "ImageService.hpp"
#include "MediaJobService.hpp"
#include "VideoService.hpp"

class MediaJobDispatcher
{
public:
    MediaJobDispatcher(
        MediaJobService& jobService,
        ImageService& imageService,
        VideoService& videoService,
        AudioService& audioService);
    ~MediaJobDispatcher();

    void enqueue(const MediaJobService::JobRecord& job);

private:
    void workerLoop();
    void processJob(const MediaJobService::JobRecord& job);

    MediaJobService& mJobService;
    ImageService& mImageService;
    VideoService& mVideoService;
    AudioService& mAudioService;

    std::mutex mMutex;
    std::condition_variable mCv;
    std::queue<MediaJobService::JobRecord> mQueue;
    bool mStop = false;
    std::thread mWorker;
};
