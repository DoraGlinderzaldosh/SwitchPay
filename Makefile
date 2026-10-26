CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Werror -g -O1
SANITIZERS := -fsanitize=address,undefined

BUILD_DIR := build
COMMON_DIR := common

.PHONY: all debug release tsan clean

all: debug

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

debug: | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SANITIZERS) \
		-fno-omit-frame-pointer \
		$(COMMON_DIR)/main.cpp \
		-o $(BUILD_DIR)/debug

release: | $(BUILD_DIR)
	$(CXX) -std=c++20 -Wall -Wextra -O2 \
		$(COMMON_DIR)/main.cpp \
		-o $(BUILD_DIR)/release

tsan: | $(BUILD_DIR)
	$(CXX) -std=c++20 -Wall -Wextra -g -O1 \
		-fsanitize=thread \
		$(COMMON_DIR)/main.cpp \
		-o $(BUILD_DIR)/tsan

clean:
	rm -rf $(BUILD_DIR)