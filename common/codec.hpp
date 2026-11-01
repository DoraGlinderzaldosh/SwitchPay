#pragma once 

#include <vector>
#include "message.hpp"
#include <cstdint>
#include <expected>
#include <string>

using Bytes = std::vector<std::uint8_t>;

std::expected<Bytes, std::string>
encode(const Message& msg);

std::expected<Message, std::string>
decode(const Bytes& bytes);