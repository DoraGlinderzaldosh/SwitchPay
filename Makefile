CXX := g++
CXXFLAGS := -std=c++23 -Wall -Wextra -Werror -g -O1
SANITIZERS := -fsanitize=address,undefined

BUILD_DIR := build
COMMON_DIR := common
TEST_DIR := tests

.PHONY: all debug release tsan test-codec clean

all: debug

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

debug: | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SANITIZERS) \
		-fno-omit-frame-pointer \
		$(COMMON_DIR)/main.cpp \
		-o $(BUILD_DIR)/debug

release: | $(BUILD_DIR)
	$(CXX) -std=c++23 -Wall -Wextra -O2 \
		$(COMMON_DIR)/main.cpp \
		-o $(BUILD_DIR)/release

tsan: | $(BUILD_DIR)
	$(CXX) -std=c++23 -Wall -Wextra -g -O1 \
		-fsanitize=thread \
		$(COMMON_DIR)/main.cpp \
		-o $(BUILD_DIR)/tsan

$(BUILD_DIR)/codec_test: $(TEST_DIR)/codec_test.cpp \
                         $(COMMON_DIR)/codec.cpp \
                         $(COMMON_DIR)/codec.hpp \
                         $(COMMON_DIR)/message.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SANITIZERS) \
		-fno-omit-frame-pointer \
		$(TEST_DIR)/codec_test.cpp \
		$(COMMON_DIR)/codec.cpp \
		-o $(BUILD_DIR)/codec_test

test-codec: $(BUILD_DIR)/codec_test
	./$(BUILD_DIR)/codec_test

clean:
	rm -rf $(BUILD_DIR)