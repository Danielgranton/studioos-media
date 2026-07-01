#pragma once

#include <string>

class VideoService {
    public:
        std::string compress(const std::string& path);

        std::string thumbnail(const std::string& path);

        std::string extractAudio(const std::string& path);

};