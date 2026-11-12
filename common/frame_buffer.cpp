#include "frame_buffer.hpp"

void FrameBuffer::fail(const  string& error) {
    failed_ = true;
    error_ = error;
}

bool FrameBuffer::failed() const {
    return failed_;
}

void FrameBuffer::compact() {
    if(readPos == 0) return;

    if(readPos == buffer__.size()) {
        buffer__.clear();
        readPos = 0;
        return;
    }

    if(readPos >= MAX_BUFFER_SIZE/2) {
        buffer__.erase(buffer__.begin(), buffer__.begin()+readPos);
        readPos = 0;
    }
}

void FrameBuffer::append(const Bytes& bytes) {
    if(failed_ || bytes.empty()) return;

    compact();
    const size_t unread = buffer__.size()-readPos;

    if(bytes.size() > MAX_BUFFER_SIZE-unread) {
        fail("Frame Buffer Capacity Exceeded");
        return;
    }

    buffer__.insert(buffer__.end(), bytes.begin(), bytes.end());
}

FrameResult FrameBuffer::next() {
    if(failed_) return unexpected(error_);

    const size_t available = buffer__.size()-readPos;
    if(available < 2) 
    return optional<Bytes>{};

    const uint16_t length = (static_cast<uint16_t>(buffer__[readPos]) << 8) |
                                  static_cast<uint16_t>(buffer__[readPos+1]);

    if(length == 0 || length > MAX_FRAME_SIZE) {
        fail("Invalid frame length");
        return unexpected(error_);
    }

    const size_t frameSize = 2+length;

    if(available < frameSize)
    return optional<Bytes>{};

    const auto bodyBegin = buffer__.begin() + readPos+2;
    const auto bodyEnd = bodyBegin+length;

    Bytes frame(bodyBegin, bodyEnd);
    readPos += frameSize;

    compact();

    return optional<Bytes>{move(frame)};
}

bool FrameBuffer::hasPendingData() const {
    return readPos < buffer__.size();
}