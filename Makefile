
CXX := g++
CXXFLAGS := -std=c++23 -Wall -Wextra -Werror -g -O1
RELEASE_FLAGS := -std=c++23 -Wall -Wextra -O2
SANITIZERS := -fsanitize=address,undefined
TSAN_FLAGS := -fsanitize=thread

BUILD_DIR := build
COMMON_DIR := common
TEST_DIR := tests

.PHONY: all debug release tsan issuer test test-codec test-frame-buffer clean

all: debug issuer

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/debug: | $(BUILD_DIR)

$(BUILD_DIR)/release: $(COMMON_DIR)/main.cpp | $(BUILD_DIR)
	$(CXX) $(RELEASE_FLAGS) $< -o $@

$(BUILD_DIR)/tsan: $(COMMON_DIR)/main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(TSAN_FLAGS) $< -o $@

$(BUILD_DIR)/issuer: issuer/main.cpp \
                     $(COMMON_DIR)/socket_utils.cpp \
                     $(COMMON_DIR)/frame_buffer.cpp \
                     $(COMMON_DIR)/codec.cpp \
                     $(COMMON_DIR)/socket_utils.hpp \
                     $(COMMON_DIR)/frame_buffer.hpp \
                     $(COMMON_DIR)/codec.hpp \
                     $(COMMON_DIR)/message.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(COMMON_DIR) \
		issuer/main.cpp \
		$(COMMON_DIR)/socket_utils.cpp \
		$(COMMON_DIR)/frame_buffer.cpp \
		$(COMMON_DIR)/codec.cpp \
		-o $@

$(BUILD_DIR)/codec_test: $(TEST_DIR)/codec_test.cpp \
                         $(COMMON_DIR)/codec.cpp \
                         $(COMMON_DIR)/codec.hpp \
                         $(COMMON_DIR)/message.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SANITIZERS) -fno-omit-frame-pointer \
		$(TEST_DIR)/codec_test.cpp $(COMMON_DIR)/codec.cpp -o $@

$(BUILD_DIR)/frame_buffer_test: $(TEST_DIR)/frame_buffer_test.cpp \
                                $(COMMON_DIR)/frame_buffer.cpp \
                                $(COMMON_DIR)/frame_buffer.hpp \
                                $(COMMON_DIR)/codec.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SANITIZERS) -fno-omit-frame-pointer \
		$(TEST_DIR)/frame_buffer_test.cpp $(COMMON_DIR)/frame_buffer.cpp -o $@

debug: test

release: $(BUILD_DIR)/release

tsan: $(BUILD_DIR)/tsan

issuer: $(BUILD_DIR)/issuer

test: test-codec test-frame-buffer

test-codec: $(BUILD_DIR)/codec_test
	./$<

test-frame-buffer: $(BUILD_DIR)/frame_buffer_test
	./$<

clean:
	rm -rf $(BUILD_DIR)
