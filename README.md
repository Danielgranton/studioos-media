# StudioOS Media

StudioOS Media is the C++ media processing service for StudioOS. It exposes a gRPC API used by the Spring Boot server for media jobs, callbacks, and polling.

## What it does

- accepts media jobs over gRPC
- processes audio, video, and image assets
- runs FFmpeg-based transforms
- uses OpenCV for image/video work
- stores temporary and processed assets through the configured storage layer
- reports job completion back to the server through gRPC callbacks
- supports polling so the server can recover if a callback is missed

## Tech stack

- C++23
- gRPC
- Protobuf
- FFmpeg
- OpenCV
- nlohmann_json
- CMake

## Repository layout

- `src/` - service code, processors, storage, config, utilities
- `proto/media.proto` - gRPC contract
- `generated/` - generated protobuf and gRPC sources
- `config/config.json` - local runtime configuration
- `tests/` - unit/integration-style executable tests
- `assets/` - local sample inputs and outputs

## Requirements

You need these installed locally:

- CMake 3.20 or newer
- a C++23 compiler
- gRPC and Protobuf development packages
- OpenCV development packages
- nlohmann_json
- FFmpeg available to the build/runtime where required by the processors

## Configuration

The service reads `config/config.json` at startup.

Key fields:

- `grpc.port` default: `50051`
- `storage.temp` temporary working directory
- `storage.assets` local asset root
- `storage.s3Bucket` S3 bucket name
- `storage.s3Region` S3 region
- `storage.s3EndpointUrl` custom S3 endpoint if needed
- `storage.s3Prefix` object prefix
- `storage.s3UsePathStyle` path-style access toggle
- `image.quality` output quality for image processing

## Run locally

Build first, then run the binary from `build/`:

```bash
cmake -S . -B build
cmake --build build
./build/studioos-media
```

The service listens on `0.0.0.0:50051` by default.

On first launch, if `config/config.json` is missing or unreadable, the app will create a default config file.

## Protobuf and gRPC

The contract lives in `proto/media.proto`.

If you change the proto, regenerate the C++ sources in `generated/` before rebuilding.

The server-side contract includes:

- `MediaService`
  - health
  - media processing operations
  - `SubmitMediaJob`
  - `GetMediaJob`
- `MediaCallbackService`
  - `ReportMediaJob`

## Build and test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

You can also run the individual test executables under `build/`.

## Supported workflow

StudioOS Server submits jobs with:

- `assetReference`
- `operation`
- `parametersJson`

The media service processes the job, stores the result reference, then:

- calls back to the server over gRPC for the fast path
- keeps job state available for polling as a fallback

## Status

The service is in active development and is intended to be the dedicated media engine for the StudioOS server. Keep the proto, generated code, and CMake sources aligned whenever you change the contract.
