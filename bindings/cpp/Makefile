CXX ?= clang++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic
LDFLAGS ?=

BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
ROOT_DIR := ../..
CATCH_SRC := $(ROOT_DIR)/third_party/catch2/catch_amalgamated.cpp
JSON_TEST := $(BIN_DIR)/json_test

.PHONY: test build clean help

test: $(JSON_TEST)
	$(JSON_TEST)

build: $(JSON_TEST)

$(JSON_TEST): json_test.cpp backend.hpp json.hpp $(CATCH_SRC)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I. -I$(ROOT_DIR)/third_party json_test.cpp $(CATCH_SRC) -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "StateSpec C++ binding targets:"
	@echo "  make test   Build and run C++ binding tests"
	@echo "  make build  Build C++ binding test binaries"
	@echo "  make clean  Remove C++ binding build outputs"
	@echo ""
	@echo "Variables:"
	@echo "  CXX=$(CXX)"
	@echo "  CXXFLAGS=$(CXXFLAGS)"
