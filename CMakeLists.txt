cmake_minimum_required(VERSION 3.20)

project(studioos-media)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

add_executable(
    studioos-media
    src/main.cpp
    src/server/MediaServer.cpp

    generated/media.pb.cc
    generated/media.grpc.pb.cc
)

target_link_libraries(
    gRPC::grpc++
    studioos-media
    protobuf::libprotobuf
    )

target_include_directories(
    studioos-media PRIVATE
    generated
    src
)