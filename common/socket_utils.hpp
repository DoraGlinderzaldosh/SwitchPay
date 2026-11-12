#pragma once
#include "frame_buffer.hpp"

expected<void, string> writeAll(int fd, const Bytes& bytes);

//just adds 2 byte length prefix
//may be redundant
expected<void, string> writeFrame(int fd, const Bytes& body);

expected<optional<Bytes>, string> readFrame(int fd, FrameBuffer& buffer);

expected<int, string> connectTo(const string& host, const string& port);