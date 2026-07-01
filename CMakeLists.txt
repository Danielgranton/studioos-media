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