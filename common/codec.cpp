#include "codec.hpp"

namespace {
    void writeU8(Bytes& out, std::uint8_t value) {
        out.push_back(value);
    }

    void writeU16(Bytes& out, std::uint16_t value) {
        out.push_back(static_cast<std::uint8_t>((value>>8)& 0xFF));
        out.push_back(static_cast<uint8_t>(value & 0xFF));
    }
    
    void writeU32(Bytes& out, std::uint32_t value) {
        out.push_back(static_cast<std::uint8_t>((value>>24)& 0xFF));
        out.push_back(static_cast<std::uint8_t>((value>>16)& 0xFF));
        out.push_back(static_cast<std::uint8_t>((value>>8)& 0xFF));
        out.push_back(static_cast<uint8_t>(value & 0xFF));
    }

    class Reader {
        private:
        const Bytes& bytes_;
        std::size_t pos_ = 0;

        public:
        explicit Reader(const Bytes& bytes) : bytes_(bytes) {}

        std::expected<std::uint8_t, std::string> readU8() {
            if(pos_ >= bytes_.size())
            return std::unexpected("Unexpected end of body");

            return bytes_[pos_++];
        }

        std::expected<std::uint16_t, std::string> readU16() {
            if(bytes_.size() - pos_ < 2)
            return std::unexpected("Unexpected end of body");

            std::uint16_t value = (static_cast<std::uint16_t>(bytes_[pos_] << 8)) |
                                  (static_cast<std::uint16_t>(bytes_[pos_+1]));

            pos_ += 2;
            return value;
        }

        std::expected<std::uint32_t, std::string> readU32() {
            if (bytes_.size() - pos_ < 4) {
                return std::unexpected("Not enough bytes for uint32");
            }

            std::uint32_t value =
                (static_cast<std::uint32_t>(bytes_[pos_]) << 24) |
                (static_cast<std::uint32_t>(bytes_[pos_ + 1]) << 16) |
                (static_cast<std::uint32_t>(bytes_[pos_ + 2]) << 8) |
                static_cast<std::uint32_t>(bytes_[pos_ + 3]);

            pos_ += 4;
            return value;
        }

        bool finished() const {
            return pos_ == bytes_.size();
        }
    };
}