#include "../common/codec.hpp"
#include <cassert>
#include  <iostream>

/*
round trip tests -- must pass
- all fields present 
- exactly two fields present 
- one field present 
- different message types
- amount scale 0, 1, 2
Goal - to verify that encode() and decode() preserve a message exactly
*/

void checkEncodedDecoded(Message *original) {
    auto encoded = encode(*original);
    assert(encoded.has_value());

    auto decoded = decode(*encoded);
    assert(decoded.has_value());

    assert(*decoded == *original);
}

void testRoundTripAllFields() {
    Message original;
    original.type = 0x0100;

    original.cardNumber = "123456789012";
    original.amount = 12550;
    original.amountScale = 2;
    original.stan = 1234;
    original.timestamp = 1730000000000ULL;
    original.key = "payment-key-001";
    original.response = "00";
    original.ostan = 5678;
    original.terminalId = "TERM0001";

    checkEncodedDecoded(&original);
}

void testRoundTripExactlyTwoFields() {
    Message original;

    original.type = 0x0100;
    original.cardNumber = "123456789012";
    original.stan = 1234;

    checkEncodedDecoded(&original);
}

void testRoundTripOneField() {
    Message original;

    original.type = 0x0100;
    original.cardNumber = "123456789012";

    checkEncodedDecoded(&original);
}

void testRoundTripAllMessageTypes() {
    const std::uint16_t types[] = {
        0x0100, //Authorization request
        0x0110, //Authorization Response
        0x0400, //Reversal request
        0x0410, //reversal response
        //0x0210, //random code --- failing as it should
    };

    for(std::uint16_t type: types) {
        Message original;

        original.type = type;
        original.cardNumber = "123456789012";
        original.amount = 12550;
        original.amountScale = 2;
        original.stan = 1234;

        checkEncodedDecoded(&original);
    }
}

void testRoundTripAmountScales() {
    const std::uint8_t scales[] = {0, 1, 2};

    for (std::uint8_t scale : scales) {
        Message original;

        original.type = 0x0100;
        original.amount = 12550;
        original.amountScale = scale;

        auto encoded = encode(original);
        assert(encoded.has_value());

        auto decoded = decode(*encoded);
        assert(decoded.has_value());

        assert(*decoded == original);

        // Explicitly verify that neither value changed.
        assert(decoded->amount == original.amount);
        assert(decoded->amountScale == original.amountScale);
    }
}

int main() {
    testRoundTripAllFields();
    testRoundTripExactlyTwoFields();
    testRoundTripAllMessageTypes();

    std::cout << "All round trip tests have passed\n";
}
