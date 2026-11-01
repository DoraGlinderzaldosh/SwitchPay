#include "../common/codec.hpp"
#include <cassert>
#include  <iostream>
#include <cstdint>
#include <string>
#include <vector>

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

void testMinimumAndMaximumBodySizes() {
    Message minimum;
    minimum.type = 0x0100;

    auto minEncoded = encode(minimum);
    assert(minEncoded.has_value());
    assert(minEncoded->size() == 3);

    auto minDecoded = decode(*minEncoded);
    assert(minDecoded.has_value());
    assert(*minDecoded == minimum);

    Message maximum;
    maximum.type = 0x0100;
    maximum.cardNumber = std::string(19, '1');
    maximum.amount = (1ULL << 56) - 1;
    maximum.amountScale = 2;
    maximum.stan = UINT32_MAX;
    maximum.timestamp = UINT64_MAX;
    maximum.key = std::string(64, 'K');
    maximum.response = "00";
    maximum.ostan = UINT32_MAX;
    maximum.terminalId = std::string(16, 'T');

    auto maxEncoded = encode(maximum);
    assert(maxEncoded.has_value());
    assert(maxEncoded->size() == 131);

    auto maxDecoded = decode(*maxEncoded);
    assert(maxDecoded.has_value());
    assert(*maxDecoded == maximum);
}

/*
Malformed input tests -- must return errors
*/

void expectDecodeError(const Bytes& body) {
    auto result = decode(body);

    if (result.has_value()) {
        std::cerr << "Unexpectedly accepted input ("
                  << body.size() << " bytes): ";

        for (std::uint8_t byte : body) {
            std::cerr << std::hex
                      << static_cast<unsigned>(byte) << ' ';
        }

        std::cerr << std::dec << '\n';
        assert(false && "Expected decode() to return an error");
    }
}

void expectEncodeError(const Message& msg) {
    auto result = encode(msg);

    if (result.has_value()) {
        std::cerr << "Unexpectedly encoded invalid message\n";
        assert(false && "Expected encode() to return an error");
    }
}

void appendU16(Bytes& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value >> 8));
    bytes.push_back(static_cast<std::uint8_t>(value));
}

Bytes makeBody(std::uint16_t type, std::uint8_t bitmap) {
    Bytes body;
    appendU16(body, type);
    body.push_back(bitmap);
    return body;
}


void testInvalidBodyStructure() {
    // Empty body.
    expectDecodeError(Bytes{});

    // One-byte and two-byte bodies.
    expectDecodeError(Bytes{0x01});
    expectDecodeError(Bytes{0x01, 0x00});

    // Body larger than 1024 bytes.
    expectDecodeError(Bytes(1025, 0x00));

    // Unsupported message type: 0x9999.
    Bytes unsupported;
    appendU16(unsupported, 0x9999);
    unsupported.push_back(0x00);
    expectDecodeError(unsupported);

    // Truncated message header: bitmap selects STAN,
    // but no STAN payload is supplied.
    expectDecodeError(makeBody(0x0100, 0x20));

    // Unexpected trailing byte after a structurally valid empty message.
    Bytes trailing = makeBody(0x0100, 0x00);
    trailing.push_back(0xAA);
    expectDecodeError(trailing);
}


void testInvalidCardNumber() {
    // Fewer than 12 digits.
    Message tooShort;
    tooShort.type = 0x0100;
    tooShort.cardNumber = "12345678901";
    expectEncodeError(tooShort);

    // More than 19 digits.
    Message tooLong;
    tooLong.type = 0x0100;
    tooLong.cardNumber = "12345678901234567890";
    expectEncodeError(tooLong);

    // Non-digit character.
    Message nonDigits;
    nonDigits.type = 0x0100;
    nonDigits.cardNumber = "12345678901A";
    expectEncodeError(nonDigits);

    // Length prefix present, but card data is truncated.
    Bytes truncated = makeBody(0x0100, 0x80);
    truncated.push_back(12);
    truncated.insert(truncated.end(), {'1', '2', '3'});
    expectDecodeError(truncated);

    // Missing card-number length byte.
    expectDecodeError(makeBody(0x0100, 0x80));
}


void testInvalidAmount() {
    // Scale greater than 2.
    Message badScale;
    badScale.type = 0x0100;
    badScale.amount = 100;
    badScale.amountScale = 3;
    expectEncodeError(badScale);

    // Amount field selected, but scale byte is missing.
    expectDecodeError(makeBody(0x0100, 0x40));

    // Scale is present, but only three of the seven amount bytes exist.
    Bytes truncatedAmount = makeBody(0x0100, 0x40);
    truncatedAmount.push_back(2);
    truncatedAmount.insert(
        truncatedAmount.end(), {0x00, 0x00, 0x01});
    expectDecodeError(truncatedAmount);

    // Amount without scale.
    Message noScale;
    noScale.type = 0x0100;
    noScale.amount = 100;
    expectEncodeError(noScale);

    // Scale without amount.
    Message noAmount;
    noAmount.type = 0x0100;
    noAmount.amountScale = 2;
    expectEncodeError(noAmount);

    // Amount exceeds the 56-bit maximum.
    Message tooLarge;
    tooLarge.type = 0x0100;
    tooLarge.amount = (1ULL << 56);
    tooLarge.amountScale = 2;
    expectEncodeError(tooLarge);
}


void testInvalidVariableLengthFields() {
    // Key longer than 64 bytes.
    Message longKey;
    longKey.type = 0x0100;
    longKey.key = std::string(65, 'K');
    expectEncodeError(longKey);

    // Terminal ID longer than 16 bytes.
    Message longTerminal;
    longTerminal.type = 0x0100;
    longTerminal.terminalId = std::string(17, 'T');
    expectEncodeError(longTerminal);

    // Key length says 5 bytes, but only 2 are present.
    Bytes shortKey = makeBody(0x0100, 0x08);
    shortKey.push_back(5);
    shortKey.insert(shortKey.end(), {'A', 'B'});
    expectDecodeError(shortKey);

    // Terminal ID length says 4 bytes, but only 1 is present.
    Bytes shortTerminal = makeBody(0x0100, 0x01);
    shortTerminal.push_back(4);
    shortTerminal.push_back('T');
    expectDecodeError(shortTerminal);

    // Missing key length prefix.
    expectDecodeError(makeBody(0x0100, 0x08));

    // Missing terminal ID length prefix.
    expectDecodeError(makeBody(0x0100, 0x01));
}


void testInvalidFixedLengthFields() {
    // STAN: only 3 of 4 bytes.
    Bytes shortStan = makeBody(0x0100, 0x20);
    shortStan.insert(shortStan.end(), {0x00, 0x00, 0x01});
    expectDecodeError(shortStan);

    // Timestamp: only 7 of 8 bytes.
    Bytes shortTimestamp = makeBody(0x0100, 0x10);
    shortTimestamp.insert(
        shortTimestamp.end(),
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01});
    expectDecodeError(shortTimestamp);

    // Original STAN: only 3 of 4 bytes.
    Bytes shortOriginalStan = makeBody(0x0400, 0x02);
    shortOriginalStan.insert(
        shortOriginalStan.end(), {0x00, 0x00, 0x01});
    expectDecodeError(shortOriginalStan);

    // Response code: only 1 of 2 bytes.
    Bytes shortResponse = makeBody(0x0110, 0x04);
    shortResponse.push_back('0');
    expectDecodeError(shortResponse);

    // Unknown response code.
    Bytes unknownResponse = makeBody(0x0110, 0x04);
    unknownResponse.insert(unknownResponse.end(), {'9', '9'});
    expectDecodeError(unknownResponse);
}


int main() {
    testRoundTripAllFields();
    testRoundTripExactlyTwoFields();
    testRoundTripOneField();
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
    testMinimumAndMaximumBodySizes();
    std::cout << "All valid boundary value tests have passed\n";

    testInvalidBodyStructure();
    testInvalidCardNumber();
    testInvalidAmount();
    testInvalidVariableLengthFields();
    testInvalidFixedLengthFields();
    std::cout << "All malformed-input tests have passed\n";

    return 0;
}
