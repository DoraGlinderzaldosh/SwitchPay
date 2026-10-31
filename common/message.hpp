
#pragma once

#include <cstdint>
#include <optional>
#include <string>

struct Message {
    std::uint16_t type{};

    std::optional<std::string> cardNumber;
    std::optional<std::uint64_t> amount;
    std::optional<std::uint32_t> stan;
    std::optional<std::uint64_t> timestamp;
    std::optional<std::string> key;
    std::optional<std::string> response;
    std::optional<std::uint32_t> ostan;
    std::optional<std::string> terminalId;
    std::optional<std::uint8_t> amountScale;

    bool operator==(const Message&) const = default;
};
