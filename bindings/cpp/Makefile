CXX ?= clang++
CLANG_FORMAT ?= clang-format
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic
LDFLAGS ?=

BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
ROOT_DIR := ../..
CATCH_SRC := $(ROOT_DIR)/third_party/catch2/catch_amalgamated.cpp
JSON_TEST := $(BIN_DIR)/json_test
FORMAT_FILES := $(wildcard *.hpp) $(wildcard *.cpp) generated/external_system_call_adapters.hpp.tmpl generated/external_system_metadata_runtime.hpp.tmpl generated/external_system_runtime.hpp.tmpl $(wildcard api/*.hpp.tmpl) $(wildcard api/*.cpp.tmpl) $(wildcard worker/*.hpp.tmpl) $(wildcard worker/*.cpp.tmpl) $(wildcard memory/*.hpp) $(wildcard runtime/*.hpp)

.PHONY: test build format clean help

test: $(JSON_TEST)
	$(JSON_TEST)

build: $(JSON_TEST)

format:
	$(CLANG_FORMAT) -i $(FORMAT_FILES)

$(JSON_TEST): json_test.cpp backend.hpp external_system.hpp feature_flag.hpp json.hpp log.hpp metric.hpp $(wildcard memory/*.hpp) $(wildcard runtime/*.hpp) $(CATCH_SRC)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I. -I$(ROOT_DIR)/third_party json_test.cpp $(CATCH_SRC) -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "StateSpec C++ binding targets:"
	@echo "  make test   Build and run C++ binding tests"
	@echo "  make build  Build C++ binding test binaries"
	@echo "  make format Format C++ binding source files"
	@echo "  make clean  Remove C++ binding build outputs"
	@echo ""
	@echo "Variables:"
	@echo "  CXX=$(CXX)"
	@echo "  CLANG_FORMAT=$(CLANG_FORMAT)"
	@echo "  CXXFLAGS=$(CXXFLAGS)"
