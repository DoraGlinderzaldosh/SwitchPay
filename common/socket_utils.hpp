#pragma once
#include "frame_buffer.hpp"

std::expected<void, std::string> writeAll(int fd, const Bytes& bytes);

//just ads 2 byte length prefix
//may be redundant
std::expected<void, std::string> writeFrame(int fd, const Bytes& body);

std::expected<std::optional<Bytes>, std::string> readFrame(int fd, FrameBuffer& buffer);

std::expected<int, std::string> connectTo(const std::string& host, const std::string& port);