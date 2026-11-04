#pragma once 
#include "codec.hpp"

constexpr std::size_t MAX_FRAME_SIZE = 1024;
constexpr std::size_t MAX_BUFFER_SIZE = 2*(MAX_FRAME_SIZE+2);

using FrameResult = std::expected<std::optional<Bytes>, std::string>;

class FrameBuffer {
    public:
    void append(const Bytes& bytes);

    FrameResult next();

    bool failed() const;

    private:
    Bytes buffer__;
    std::size_t readPos = 0;
    bool failed_ = false;
    std::string error_;

    void compact();
    void fail(const std::string& err);
};