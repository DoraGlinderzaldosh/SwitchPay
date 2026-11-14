
#pragma once

#include <cstdint>
#include <optional>
#include <string>

using namespace std;

struct Message {
    uint16_t type{};

    optional<string> cardNumber;
    optional<uint64_t> amount;
    optional<uint32_t> stan;
    optional<uint64_t> timestamp;
    optional<string> key;
    optional<string> response;
    optional<uint32_t> ostan;
    optional<string> terminalId;
    optional<uint8_t> amountScale;

    bool operator==(const Message&) const = default;
};
