#pragma once 

#include <vector>
#include "message.hpp"
#include <cstdint>
#include <expected>
#include <string>

using namespace std;

using Bytes = vector<uint8_t>;

expected<Bytes, string>
encode(const Message& msg);

expected<Message, string>
decode(const Bytes& bytes);