cmake_minimum_required(VERSION 3.20)

project(studioos-media)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(OpenCV REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(
    studioos-media
    src/main.cpp
    src/grpc/MediaServer.cpp

    generated/media.pb.cc
    generated/media.grpc.pb.cc

    src/media/ImageProcessor.cpp
    src/media/VideoProcessor.cpp

    src/ffmpeg/CommandRunner.cpp
    src/ffmpeg/FFmpeg.cpp
    src/ffmpeg/VideoEncoder.cpp
    src/ffmpeg/AudioEncoder.cpp
    src/ffmpeg/ThumbnailGenerator.cpp
    src/ffmpeg/VideoConverter.cpp

    src/services/ImageService.cpp
    src/services/VideoService.cpp
    src/services/AudioService.cpp
    src/services/MediaJobService.cpp
    src/services/MediaJobDispatcher.cpp
    src/media/AudioProcessor.cpp
    src/services/StreamingService.cpp
    src/services/AdService.cpp
    src/storage/S3Storage.cpp
    src/config/config.cpp
    src/utils/Logger.cpp
    src/utils/FileUtils.cpp
    src/utils/Timer.cpp
    )

target_link_libraries(
    studioos-media
    PRIVATE
        protobuf::libprotobuf
        gRPC::grpc++
        ${OpenCV_LIBS}
        nlohmann_json::nlohmann_json
    )

target_include_directories(
    studioos-media 
    PRIVATE
        generated
        src
        config
        ${OpenCV_INCLUDE_DIRS}
)

add_executable(
    video-processor-test
    tests/video_processor_test.cpp
    src/media/VideoProcessor.cpp
    src/services/VideoService.cpp
    src/services/StreamingService.cpp
    src/config/config.cpp
    src/ffmpeg/CommandRunner.cpp
    src/ffmpeg/FFmpeg.cpp
    src/ffmpeg/VideoEncoder.cpp
    src/ffmpeg/AudioEncoder.cpp
    src/ffmpeg/ThumbnailGenerator.cpp
    src/ffmpeg/VideoConverter.cpp
    src/utils/FileUtils.cpp
    src/utils/Timer.cpp
    src/utils/Logger.cpp
)

target_link_libraries(
    video-processor-test
    PRIVATE
        ${OpenCV_LIBS}
        nlohmann_json::nlohmann_json
)

target_include_directories(
    video-processor-test
    PRIVATE
        src
        generated
        config
        ${OpenCV_INCLUDE_DIRS}
)

add_executable(
    image-processor-test
    tests/image_processor_test.cpp
    src/media/ImageProcessor.cpp
    src/config/config.cpp
    src/ffmpeg/CommandRunner.cpp
    src/ffmpeg/FFmpeg.cpp
    src/ffmpeg/VideoEncoder.cpp
    src/ffmpeg/AudioEncoder.cpp
    src/ffmpeg/ThumbnailGenerator.cpp
    src/ffmpeg/VideoConverter.cpp
    src/utils/FileUtils.cpp
    src/utils/Timer.cpp
    src/utils/Logger.cpp
)

target_link_libraries(
    image-processor-test
    PRIVATE
        ${OpenCV_LIBS}
        nlohmann_json::nlohmann_json
)

target_include_directories(
    image-processor-test
    PRIVATE
        src
        generated
        config
        ${OpenCV_INCLUDE_DIRS}
)

add_executable(
    audio-processor-test
    tests/audio_processor_test.cpp
    src/media/AudioProcessor.cpp
    src/config/config.cpp
    src/ffmpeg/CommandRunner.cpp
    src/ffmpeg/FFmpeg.cpp
    src/utils/FileUtils.cpp
    src/utils/Logger.cpp
    src/utils/Timer.cpp
)

target_link_libraries(
    audio-processor-test
    PRIVATE
        nlohmann_json::nlohmann_json
)

target_include_directories(
    audio-processor-test
    PRIVATE
        src
        generated
        config
        ${OpenCV_INCLUDE_DIRS}
)

add_executable(
    streaming-service-test
    tests/streaming_service_test.cpp
    src/services/StreamingService.cpp
    src/ffmpeg/CommandRunner.cpp
    src/ffmpeg/FFmpeg.cpp
    src/utils/FileUtils.cpp
    src/utils/Logger.cpp
    src/utils/Timer.cpp
    src/config/config.cpp
)

target_link_libraries(
    streaming-service-test
    PRIVATE
        nlohmann_json::nlohmann_json
)

target_include_directories(
    streaming-service-test
    PRIVATE
        src
        generated
        config
        ${OpenCV_INCLUDE_DIRS}
)

add_executable(
    ad-service-test
    tests/ad_service_test.cpp
    src/services/AdService.cpp
    src/services/StreamingService.cpp
    src/media/ImageProcessor.cpp
    src/media/VideoProcessor.cpp
    src/ffmpeg/CommandRunner.cpp
    src/ffmpeg/FFmpeg.cpp
    src/ffmpeg/VideoEncoder.cpp
    src/ffmpeg/AudioEncoder.cpp
    src/ffmpeg/ThumbnailGenerator.cpp
    src/ffmpeg/VideoConverter.cpp
    src/config/config.cpp
    src/utils/FileUtils.cpp
    src/utils/Logger.cpp
    src/utils/Timer.cpp
)

target_link_libraries(
    ad-service-test
    PRIVATE
        ${OpenCV_LIBS}
        nlohmann_json::nlohmann_json
)

target_include_directories(
    ad-service-test
    PRIVATE
        src
        generated
        config
        ${OpenCV_INCLUDE_DIRS}
)

add_executable(
    media-job-service-test
    tests/media_job_service_test.cpp
    src/services/MediaJobService.cpp
)

target_link_libraries(
    media-job-service-test
    PRIVATE
        nlohmann_json::nlohmann_json
)

target_include_directories(
    media-job-service-test
    PRIVATE
        src
        generated
        config
        ${OpenCV_INCLUDE_DIRS}
)

add_executable(
    s3-storage-test
    tests/s3_storage_test.cpp
    src/storage/S3Storage.cpp
    src/config/config.cpp
    src/ffmpeg/CommandRunner.cpp
    src/utils/FileUtils.cpp
)

target_link_libraries(
    s3-storage-test
    PRIVATE
        nlohmann_json::nlohmann_json
)

target_include_directories(
    s3-storage-test
    PRIVATE
        src
        generated
        config
        ${OpenCV_INCLUDE_DIRS}
)

add_executable(
    grpc-ad-service-test
    tests/grpc_ad_service_test.cpp
    src/grpc/MediaServer.cpp
    src/services/AdService.cpp
    src/services/MediaJobService.cpp
    src/services/MediaJobDispatcher.cpp
    src/services/AudioService.cpp
    src/services/ImageService.cpp
    src/services/VideoService.cpp
    src/services/StreamingService.cpp
    src/media/AudioProcessor.cpp
    src/media/ImageProcessor.cpp
    src/media/VideoProcessor.cpp
    src/ffmpeg/CommandRunner.cpp
    src/ffmpeg/FFmpeg.cpp
    src/ffmpeg/VideoEncoder.cpp
    src/ffmpeg/AudioEncoder.cpp
    src/ffmpeg/ThumbnailGenerator.cpp
    src/ffmpeg/VideoConverter.cpp
    src/utils/FileUtils.cpp
    src/utils/Logger.cpp
    src/utils/Timer.cpp
    src/config/config.cpp
    src/storage/S3Storage.cpp
    generated/media.pb.cc
    generated/media.grpc.pb.cc
)

target_link_libraries(
    grpc-ad-service-test
    PRIVATE
        protobuf::libprotobuf
        gRPC::grpc++
        ${OpenCV_LIBS}
        nlohmann_json::nlohmann_json
)

target_include_directories(
    grpc-ad-service-test
    PRIVATE
        generated
        src
        config
        ${OpenCV_INCLUDE_DIRS}
)

add_executable(
    grpc-media-job-test
    tests/grpc_media_job_test.cpp
    src/grpc/MediaServer.cpp
    src/services/AdService.cpp
    src/services/MediaJobService.cpp
    src/services/MediaJobDispatcher.cpp
    src/services/AudioService.cpp
    src/services/ImageService.cpp
    src/services/VideoService.cpp
    src/services/StreamingService.cpp
    src/media/AudioProcessor.cpp
    src/media/ImageProcessor.cpp
    src/media/VideoProcessor.cpp
    src/ffmpeg/CommandRunner.cpp
    src/ffmpeg/FFmpeg.cpp
    src/ffmpeg/VideoEncoder.cpp
    src/ffmpeg/AudioEncoder.cpp
    src/ffmpeg/ThumbnailGenerator.cpp
    src/ffmpeg/VideoConverter.cpp
    src/utils/FileUtils.cpp
    src/utils/Logger.cpp
    src/utils/Timer.cpp
    src/config/config.cpp
    src/storage/S3Storage.cpp
    generated/media.pb.cc
    generated/media.grpc.pb.cc
)

target_link_libraries(
    grpc-media-job-test
    PRIVATE
        protobuf::libprotobuf
        gRPC::grpc++
        ${OpenCV_LIBS}
        nlohmann_json::nlohmann_json
)

target_include_directories(
    grpc-media-job-test
    PRIVATE
        generated
        src
        config
        ${OpenCV_INCLUDE_DIRS}
)

add_executable(
    grpc-media-callback-test
    tests/grpc_media_callback_test.cpp
    src/grpc/MediaServer.cpp
    src/services/AdService.cpp
    src/services/MediaJobService.cpp
    src/services/MediaJobDispatcher.cpp
    src/services/AudioService.cpp
    src/services/ImageService.cpp
    src/services/VideoService.cpp
    src/services/StreamingService.cpp
    src/media/AudioProcessor.cpp
    src/media/ImageProcessor.cpp
    src/media/VideoProcessor.cpp
    src/ffmpeg/CommandRunner.cpp
    src/ffmpeg/FFmpeg.cpp
    src/ffmpeg/VideoEncoder.cpp
    src/ffmpeg/AudioEncoder.cpp
    src/ffmpeg/ThumbnailGenerator.cpp
    src/ffmpeg/VideoConverter.cpp
    src/utils/FileUtils.cpp
    src/utils/Logger.cpp
    src/utils/Timer.cpp
    src/config/config.cpp
    src/storage/S3Storage.cpp
    generated/media.pb.cc
    generated/media.grpc.pb.cc
)

target_link_libraries(
    grpc-media-callback-test
    PRIVATE
        protobuf::libprotobuf
        gRPC::grpc++
        ${OpenCV_LIBS}
        nlohmann_json::nlohmann_json
)

target_include_directories(
    grpc-media-callback-test
    PRIVATE
        generated
        src
        config
        ${OpenCV_INCLUDE_DIRS}
)

enable_testing()
add_test(NAME video-processor-test COMMAND video-processor-test)
add_test(NAME image-processor-test COMMAND image-processor-test)
add_test(NAME audio-processor-test COMMAND audio-processor-test)
add_test(NAME streaming-service-test COMMAND streaming-service-test)
add_test(NAME ad-service-test COMMAND ad-service-test)
add_test(NAME grpc-ad-service-test COMMAND grpc-ad-service-test)
add_test(NAME media-job-service-test COMMAND media-job-service-test)
add_test(NAME grpc-media-job-test COMMAND grpc-media-job-test)
add_test(NAME grpc-media-callback-test COMMAND grpc-media-callback-test)
add_test(NAME s3-storage-test COMMAND s3-storage-test)
