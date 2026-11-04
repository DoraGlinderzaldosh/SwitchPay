#include "../common/frame_buffer.hpp"
#include <iostream>
#include <random>

using namespace std;
#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            cerr << "FAILED: " << #condition \
                 << " at line " << __LINE__ << '\n'; \
            exit(1); \
        } \
    } while (false)

Bytes makeFrame(const Bytes& body) {
    Bytes frame;

    uint16_t len = static_cast<uint16_t>(body.size());

    frame.push_back(static_cast<uint8_t>(len >> 8));
    frame.push_back(static_cast<uint8_t>(len & 0xFF));

    frame.insert(frame.end(), body.begin(), body.end());

    return frame;
}

void testOneByteAtATime() {
    Bytes body = {0xAA, 0xBB, 0xCC, 0xDD};
    Bytes frame = makeFrame(body);

    FrameBuffer fb;
    for(size_t i = 0; i < frame.size(); i++) {
        fb.append(Bytes{frame[i]});
        auto result = fb.next();

        CHECK(result.has_value());

        if(i!=frame.size()-1)
        CHECK(!result->has_value());
        else {
            CHECK(result->has_value());
            CHECK(result->value() == body);
        }
    }

    CHECK(!fb.failed());
}

void testTwoGluedMessages() {
    Bytes body1 = {0xAA, 0xBB};
    Bytes body2 = {0xCC, 0xDD, 0xEE};

    Bytes frame1 = makeFrame(body1);
    Bytes frame2 = makeFrame(body2);

    Bytes combined = frame1;
    combined.insert(combined.end(), frame2.begin(), frame2.end());

    FrameBuffer fb;
    fb.append(combined);
    auto result1 = fb.next();

    CHECK(result1.has_value());
    CHECK(result1->has_value());
    CHECK(result1->value() == body1);

    auto result2 = fb.next();
    CHECK(result2.has_value());
    CHECK(result2->has_value());
    CHECK(result2->value() == body2);

    auto result3 = fb.next();
    CHECK(result3.has_value());
    CHECK(!result3->has_value());

    CHECK(!fb.failed());
}

void testTwoAndHalfMessages() {
    Bytes body1 = {0xAA, 0xBB};
    Bytes body2 = {0xCC, 0xDD};
    Bytes body3 = {0xEE, 0xFF, 0x11, 0x22};

    Bytes frame1 = makeFrame(body1);
    Bytes frame2 = makeFrame(body2);
    Bytes frame3 = makeFrame(body3);

    FrameBuffer fb;

    Bytes combined = frame1;
    combined.insert(combined.end(), frame2.begin(), frame2.end());
    
    size_t half = frame3.size()/2;
    combined.insert(combined.end(), frame3.begin(), frame3.begin()+half);

    fb.append(combined);

    auto result1 = fb.next();
    CHECK(result1.has_value());
    CHECK(result1->has_value());
    CHECK(result1->value() == body1);

    auto result2 = fb.next();
    CHECK(result2.has_value());
    CHECK(result2->has_value());
    CHECK(result2->value() == body2);

    auto result3 = fb.next();
    CHECK(result3.has_value());
    CHECK(!result3->has_value());

    fb.append(Bytes(frame3.begin()+half, frame3.end()));

    auto result4 = fb.next();
    CHECK(result4.has_value());
    CHECK(result4->has_value());
    CHECK(result4->value() == body3);

    auto result5 = fb.next();
    CHECK(result5.has_value());
    CHECK(!result5->has_value());

    CHECK(!fb.failed());
}

void testOversizedLength() {
    FrameBuffer fb;
    Bytes header = {
        0xEA,
        0x60
    };

    fb.append(header);
    auto result = fb.next();
    CHECK(!result.has_value());
    CHECK(fb.failed());

    auto result2 = fb.next();
    CHECK(!result2.has_value());
}

void testZeroLength() {
    FrameBuffer fb;
    Bytes header = {
        0x00,
        0x00
    };

    fb.append(header);
    auto result = fb.next();
    CHECK(!result.has_value());
    CHECK(fb.failed());

    auto result2 = fb.next();
    CHECK(!result2.has_value());
}

void testMaximumLength() {
    Bytes body(MAX_FRAME_SIZE, 0xAB);
    Bytes frame = makeFrame(body);

    FrameBuffer fb;
    fb.append(frame);

    auto result = fb.next();
    CHECK(result.has_value());
    CHECK(result->has_value());
    CHECK(result->value() == body);

    CHECK(!fb.failed());

    auto result2 = fb.next();
    CHECK(result2.has_value());
    CHECK(!result2->has_value());

    Bytes oversizedBody(MAX_FRAME_SIZE + 1, 0xAB);
    Bytes oversizedFrame = makeFrame(oversizedBody);
    FrameBuffer fb2;
    fb2.append(oversizedFrame);
    auto result3 = fb2.next();

    CHECK(!result3.has_value());
    CHECK(fb2.failed());
}

void testTruncatedFrame() {
    Bytes body = {
        0xAA, 0xBB, 0xCC,
        0xDD, 0xEE, 0xFF
    };

    Bytes frame = makeFrame(body);
    FrameBuffer fb;

    size_t partialSize = 2 + 3;
    Bytes partial(
        frame.begin(),
        frame.begin() + partialSize
    );

    fb.append(partial);
    auto result1 = fb.next();

    CHECK(result1.has_value());
    CHECK(!result1->has_value());
    CHECK(!fb.failed());

    Bytes remaining(frame.begin() + partialSize, frame.end());

    fb.append(remaining);
    auto result2 = fb.next();
    CHECK(result2.has_value());
    CHECK(result2->has_value());
    CHECK(result2->value() == body);
    CHECK(!fb.failed());

    auto result3 = fb.next();

    CHECK(result3.has_value());
    CHECK(!result3->has_value());
}

void testRandomMessages() {
    constexpr int NUM_MESSAGES = 2000;
    mt19937 rng(42);

    uniform_int_distribution<int> sizeDist(1, MAX_FRAME_SIZE);
    uniform_int_distribution<int> byteDist(0, 255);

    uniform_int_distribution<int> chunkDist(1, 40);

    vector<Bytes> messages;
    messages.reserve(NUM_MESSAGES);

    for(int i = 0; i < NUM_MESSAGES;i++) {
        int size = sizeDist(rng);
        Bytes body(size);

        for(auto& byte: body) 
        byte = static_cast<uint8_t>(byteDist(rng));

        messages.push_back(move(body));
    }

    Bytes stream;
    for(const auto& body: messages) {
        Bytes frame = makeFrame(body);

        stream.insert(stream.end(), frame.begin(), frame.end());
    }

    FrameBuffer fb;
    size_t pos = 0;
    size_t extracted = 0;

    while(pos < stream.size()) {
        int chunkSize = chunkDist(rng);
        size_t end = min(pos+static_cast<size_t>(chunkSize), stream.size());
        Bytes chunk(stream.begin()+pos, stream.begin()+end);

        fb.append(chunk);
        pos = end;

        while(true) {
            auto result = fb.next();
            CHECK(result.has_value());
            if(!result->has_value()) break;
            CHECK(extracted < messages.size());
            CHECK(result->value() == messages[extracted]);
            extracted++;
        }

        CHECK(!fb.failed());
    }
}

int main() {
    testOneByteAtATime();
    testTwoGluedMessages();
    testTwoAndHalfMessages();
    testOversizedLength();
    testZeroLength();
    testMaximumLength();
    testTruncatedFrame();
    testRandomMessages();

    cout << "All FrameBuffer tests passed!\n";
}