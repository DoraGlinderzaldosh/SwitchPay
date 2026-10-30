
#pragma once

#include <cstdint>
#include <optional>
#include <string>

struct Message {
    std::uint16_t type{};

    std::optional<std::string> cardNumber;
    std::optional<std::uint32_t> amount;
    std::optional<std::string> stan;
    std::optional<std::string> timestamp;
    std::optional<std::string> key;
    std::optional<std::uint16_t> response;
    std::optional<std::string> ostan;
    std::optional<std::string> terminalId;

    bool operator==(const Message&) const = default;
};
