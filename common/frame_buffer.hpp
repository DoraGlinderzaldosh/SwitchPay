#pragma once 
#include "codec.hpp"

constexpr size_t MAX_FRAME_SIZE = 1024;
constexpr size_t MAX_BUFFER_SIZE = 2*(MAX_FRAME_SIZE+2);

using FrameResult = expected<optional<Bytes>, string>;

class FrameBuffer {
    public:
    void append(const Bytes& bytes);

    FrameResult next();

    bool failed() const;
    bool hasPendingData() const;

    private:
    Bytes buffer__;
    size_t readPos = 0;
    bool failed_ = false;
    string error_;

    void compact();
    void fail(const string& err);
};