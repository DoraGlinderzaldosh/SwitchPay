#include "codec.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace {

constexpr std::size_t MAX_BODY_SIZE = 1024;
constexpr std::uint64_t MAX_AMOUNT = (1ULL << 56) - 1;

constexpr std::uint8_t fieldBit(unsigned field) {
    return static_cast<std::uint8_t>(1U << (7U - field));
}

void writeU8(Bytes& out, std::uint8_t value) {
    out.push_back(value);
}

void writeU16(Bytes& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value >> 8));
    out.push_back(static_cast<std::uint8_t>(value));
}

void writeU32(Bytes& out, std::uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8)
    out.push_back(static_cast<std::uint8_t>(value >> shift));
}

void writeU56(Bytes& out, std::uint64_t value) {
    for (int shift = 48; shift >= 0; shift -= 8) 
    out.push_back(static_cast<std::uint8_t>(value >> shift));
}

void writeU64(Bytes& out, std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) 
    out.push_back(static_cast<std::uint8_t>(value >> shift));
}

void writeString(Bytes& out, const std::string& value) {
    writeU8(out, static_cast<std::uint8_t>(value.size()));
    for (unsigned char c : value)
    out.push_back(c);
}

bool allDigits(const std::string& value) {
    return !value.empty() && std::all_of(value.begin(), value.end(),[](unsigned char c) {
            return c >= '0' && c <= '9';
        }
    );
}

bool validResponseCode(const std::string& code) {
    static constexpr std::array<std::string_view, 10> codes = {
        "00", "05", "12", "13", "30",
        "51", "54", "57", "91", "96"
    };
    return std::find(codes.begin(), codes.end(), code) != codes.end();
}

bool validType(std::uint16_t type) {
    return type == 0x0100 || type == 0x0110 || type == 0x0400 || type == 0x0410;
}

class Reader {
private:
    const Bytes& bytes_;
    std::size_t pos_ = 0;

public:
    explicit Reader(const Bytes& bytes) : bytes_(bytes) {}

    std::size_t remaining() const {
        return bytes_.size() - pos_;
    }

    bool finished() const {
        return pos_ == bytes_.size();
    }

    std::expected<std::uint8_t, std::string> readU8() {
        if (remaining() < 1)
        return std::unexpected("Unexpected end of body");
        return bytes_[pos_++];
    }

    std::expected<std::uint16_t, std::string> readU16() {
        if (remaining() < 2)
        return std::unexpected("Not enough bytes for uint16");
        std::uint16_t value = (static_cast<std::uint16_t>(bytes_[pos_]) << 8) | static_cast<std::uint16_t>(bytes_[pos_ + 1]);
        pos_ += 2;
        return value;
    }

    std::expected<std::uint32_t, std::string> readU32() {
        if (remaining() < 4)
        return std::unexpected("Not enough bytes for uint32");
        std::uint32_t value = 0;
        for (int i = 0; i < 4; ++i)
        value = (value << 8) | bytes_[pos_++];
        return value;
    }

    std::expected<std::uint64_t, std::string> readU56() {
        if (remaining() < 7)
        return std::unexpected("Not enough bytes for uint56");
        std::uint64_t value = 0;
        for (int i = 0; i < 7; ++i)
        value = (value << 8) | bytes_[pos_++];
        return value;
    }

    std::expected<std::uint64_t, std::string> readU64() {
        if (remaining() < 8)
        return std::unexpected("Not enough bytes for uint64");
        std::uint64_t value = 0;
        for (int i = 0; i < 8; ++i)
        value = (value << 8) | bytes_[pos_++];
        return value;
    }

    std::expected<std::string, std::string>
    readString(std::size_t maxLength) {
        auto length = readU8();
        if (!length)
        return std::unexpected(length.error());
        if (*length > maxLength)
        return std::unexpected("String exceeds maximum length");
        if (remaining() < *length)
        return std::unexpected("Truncated string");
        std::string value;
        value.reserve(*length);
        for (std::size_t i = 0; i < *length; ++i)
        value.push_back(static_cast<char>(bytes_[pos_++]));
        return value;
    }
};
}

std::expected<Bytes, std::string>
encode(const Message& msg) {
    if (!validType(msg.type))
    return std::unexpected("Unsupported message type");
    std::uint8_t bitmap = 0;
    if (msg.cardNumber) {
        if (msg.cardNumber->size() < 12 || msg.cardNumber->size() > 19 || !allDigits(*msg.cardNumber))
        return std::unexpected("Invalid card number");
        bitmap |= fieldBit(0);
    }
    if (msg.amount) {
        if (*msg.amount > MAX_AMOUNT)
        return std::unexpected("Amount exceeds 56-bit limit");
        if (!msg.amountScale || *msg.amountScale > 2)
        return std::unexpected("Amount scale must be 0, 1, or 2");
        bitmap |= fieldBit(1);
    } else if (msg.amountScale)
    return std::unexpected("Amount scale set without amount");
    if (msg.stan)
    bitmap |= fieldBit(2);
    if (msg.timestamp)
    bitmap |= fieldBit(3);
    if (msg.key) {
        if (msg.key->size() > 64)
        return std::unexpected("Idempotency key too long");
        bitmap |= fieldBit(4);
    }
    if (msg.response) {
        if (!validResponseCode(*msg.response))
        return std::unexpected("Invalid response code");
        bitmap |= fieldBit(5);
    }
    if (msg.ostan)
    bitmap |= fieldBit(6);
    if (msg.terminalId) {
        if (msg.terminalId->size() > 16)
        return std::unexpected("Terminal ID too long");
        bitmap |= fieldBit(7);
    }
    Bytes body;
    body.reserve(64);
    writeU16(body, msg.type);
    writeU8(body, bitmap);
    if (msg.cardNumber)
    writeString(body, *msg.cardNumber);
    if (msg.amount) {
        writeU8(body, *msg.amountScale);
        writeU56(body, *msg.amount);
    }
    if (msg.stan)
    writeU32(body, *msg.stan);
    if (msg.timestamp)
    writeU64(body, *msg.timestamp);
    if (msg.key)
    writeString(body, *msg.key);
    if (msg.response) {
        body.push_back(static_cast<std::uint8_t>((*msg.response)[0]));
        body.push_back(static_cast<std::uint8_t>((*msg.response)[1]));
    }
    if (msg.ostan)
    writeU32(body, *msg.ostan);
    if (msg.terminalId)
    writeString(body, *msg.terminalId);
    if (body.size() < 3 || body.size() > MAX_BODY_SIZE)
    return std::unexpected("Invalid body size");
    return body;
}

std::expected<Message, std::string>
decode(const Bytes& body) {
    if (body.size() < 3 || body.size() > MAX_BODY_SIZE)
    return std::unexpected("Invalid body size");
    Reader reader(body);
    Message msg;
    auto type = reader.readU16();
    if (!type)
    return std::unexpected(type.error());
    if (!validType(*type))
    return std::unexpected("Unsupported message type");
    msg.type = *type;
    auto bitmapResult = reader.readU8();
    if (!bitmapResult)
    return std::unexpected(bitmapResult.error());
    const std::uint8_t bitmap = *bitmapResult;
    if (bitmap & fieldBit(0)) {
        auto value = reader.readString(19);
        if (!value) 
        return std::unexpected(value.error());
        if (value->size() < 12 || !allDigits(*value)) 
        return std::unexpected("Invalid card number");
        msg.cardNumber = *value;
    }
    if (bitmap & fieldBit(1)) {
        auto scale = reader.readU8();
        if (!scale)
        return std::unexpected(scale.error());
        if (*scale > 2)
        return std::unexpected("Invalid amount scale");
        auto amount = reader.readU56();
        if (!amount)
        return std::unexpected(amount.error());
        msg.amountScale = *scale;
        msg.amount = *amount;
    }
    if (bitmap & fieldBit(2)) {
        auto value = reader.readU32();
        if (!value)
        return std::unexpected(value.error());
        msg.stan = *value;
    }
    if (bitmap & fieldBit(3)) {
        auto value = reader.readU64();
        if (!value)
        return std::unexpected(value.error());
        msg.timestamp = *value;
    }
    if (bitmap & fieldBit(4)) {
        auto value = reader.readString(64);
        if (!value)
        return std::unexpected(value.error());
        msg.key = *value;
    }
    if (bitmap & fieldBit(5)) {
        auto first = reader.readU8();
        auto second = reader.readU8();
        if (!first || !second)
        return std::unexpected("Truncated response code");
        std::string response{static_cast<char>(*first), static_cast<char>(*second)};
        if (!validResponseCode(response)) 
        return std::unexpected("Invalid response code");
        msg.response = response;
    }
    if (bitmap & fieldBit(6)) {
        auto value = reader.readU32();
        if (!value)
        return std::unexpected(value.error());
        msg.ostan = *value;
    }
    if (bitmap & fieldBit(7)) {
        auto value = reader.readString(16);
        if (!value)
        return std::unexpected(value.error());
        msg.terminalId = *value;
    }
    if (!reader.finished())
    return std::unexpected("Unexpected trailing bytes");
    return msg;
}