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

    src/services/ImageService.cpp
    src/services/VideoService.cpp
    src/services/AudioService.cpp
    src/services/StreamingService.cpp
    src/services/AdService.cpp
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
    src/config/config.cpp
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

enable_testing()
add_test(NAME video-processor-test COMMAND video-processor-test)
add_test(NAME image-processor-test COMMAND image-processor-test)