
#include "../common/codec.hpp"

#include <cassert>
#include <iostream>

int main() {
    Message original;

    original.type = 0x0100;
    original.cardNumber = "1234567890123456";
    original.amount = 12550;
    original.amountScale = 2;
    original.stan = 1234;
    original.timestamp = 1793440000000ULL;
    original.key = "payment-001";
    original.terminalId = "TERM0001";

    auto encoded = encode(original);
    assert(encoded.has_value());

    auto decoded = decode(*encoded);
    assert(decoded.has_value());

    assert(*decoded == original);

    std::cout << "Codec round-trip test passed!\n";
}
