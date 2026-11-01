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

/*
Valid boundary value tests
check the smallest and the largest values allowed by the protocol
- card number: 12 digts, 19 digits
- amount: 0, maximum 56 bit value 
- STAN: 0, UINT32_MAX
- timestamp: 0, UINT64_MAX
- Idempotency Key: empty, 1 byte, 64 bytes
- Response code: every permitted response code
- Original STAN: 0, UINT32_MAX
- Terminal ID: Empty, 1 byte, 16 bytes
- Body size: Minimum valid body and maximum valid body, where the content is valid
*/


void testCardNumberBoundaries() {
    Message minimum;
    minimum.type = 0x0100;
    minimum.cardNumber = "123456789012";

    checkEncodedDecoded(&minimum);

    Message maximum;
    maximum.type = 0x0100;
    maximum.cardNumber = "1234567890123456789";

    checkEncodedDecoded(&maximum);
}


void testAmountBoundaries() {
    Message zero;
    zero.type = 0x0100;
    zero.amount = 0;
    zero.amountScale = 0;

    checkEncodedDecoded(&zero);

    Message maximum;
    maximum.type = 0x0100;
    maximum.amount = (1ULL << 56) - 1;
    maximum.amountScale = 2;

    checkEncodedDecoded(&maximum);
}


void testStanBoundaries() {
    Message minimum;
    minimum.type = 0x0100;
    minimum.stan = 0;

    checkEncodedDecoded(&minimum);

    Message maximum;
    maximum.type = 0x0100;
    maximum.stan = UINT32_MAX;

    checkEncodedDecoded(&maximum);
}

void testOriginalStanBoundaries() {
    Message minimum;
    minimum.type = 0x0400;
    minimum.ostan = 0;

    checkEncodedDecoded(&minimum);

    Message maximum;
    maximum.type = 0x0400;
    maximum.ostan = UINT32_MAX;

    checkEncodedDecoded(&maximum);
}


void testTimestampBoundaries() {
    Message minimum;
    minimum.type = 0x0100;
    minimum.timestamp = 0;

    checkEncodedDecoded(&minimum);

    Message maximum;
    maximum.type = 0x0100;
    maximum.timestamp = UINT64_MAX;

    checkEncodedDecoded(&maximum);
}


void testKeyBoundaries() {
    Message empty;
    empty.type = 0x0100;
    empty.key = "";

    checkEncodedDecoded(&empty);

    Message minimum;
    minimum.type = 0x0100;
    minimum.key = "K";

    checkEncodedDecoded(&minimum);

    Message maximum;
    maximum.type = 0x0100;
    maximum.key = std::string(64, 'K');

    checkEncodedDecoded(&maximum);
}

void testAllValidResponseCodes() {
    const std::string codes[] = {
        "00", "05", "12", "13", "30",
        "51", "54", "57", "91", "96"
    };

    for (const std::string& code : codes) {
        Message original;
        original.type = 0x0110;
        original.response = code;

        checkEncodedDecoded(&original);
    }
}

void testTerminalIdBoundaries() {
    Message empty;
    empty.type = 0x0100;
    empty.terminalId = "";

    checkEncodedDecoded(&empty);

    Message minimum;
    minimum.type = 0x0100;
    minimum.terminalId = "T";

    checkEncodedDecoded(&minimum);

    Message maximum;
    maximum.type = 0x0100;
    maximum.terminalId = std::string(16, 'T');

    checkEncodedDecoded(&maximum);
}

int main() {
    testRoundTripAllFields();
    testRoundTripExactlyTwoFields();
    testRoundTripAllMessageTypes();
    testRoundTripAmountScales();
    std::cout << "All round trip tests have passed\n";

    testCardNumberBoundaries();
    testAmountBoundaries();
    testStanBoundaries();
    testOriginalStanBoundaries();
    testTimestampBoundaries();
    testKeyBoundaries();
    testAllValidResponseCodes();
    testTerminalIdBoundaries();
    std::cout << "All valid boundary value tests have passed\n";
}
