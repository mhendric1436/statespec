CXX ?= clang++
CLANG_FORMAT ?= clang-format
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic
LDFLAGS ?=

BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
OBJ_DIR := $(BUILD_DIR)/obj
ROOT_DIR := ../..
CATCH_SRC := $(ROOT_DIR)/third_party/catch2/catch_amalgamated.cpp
BINDING_TEST := $(BIN_DIR)/binding_tests
TEST_SOURCES := json_test.cpp memory_backend_registration_test.cpp schema_compatibility_test.cpp
TEST_OBJECTS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(TEST_SOURCES))
GENERATED_FORMAT_TEMPLATES := $(filter-out generated/descriptors.hpp.tmpl generated/runtime_registration.hpp.tmpl,$(wildcard generated/*.hpp.tmpl))
API_FORMAT_TEMPLATES := $(filter-out api/api_application.hpp.tmpl,$(wildcard api/*.hpp.tmpl))
WORKER_FORMAT_TEMPLATES := $(filter-out worker/worker_runtime.hpp.tmpl,$(wildcard worker/*.hpp.tmpl))
FORMAT_FILES := $(wildcard *.hpp) $(wildcard *.cpp) $(GENERATED_FORMAT_TEMPLATES) $(API_FORMAT_TEMPLATES) $(wildcard api/*.cpp.tmpl) $(WORKER_FORMAT_TEMPLATES) $(wildcard worker/*.cpp.tmpl) $(wildcard memory/*.hpp) $(wildcard runtime/*.hpp)

.PHONY: test build format clean help

test: $(BINDING_TEST)
	$(BINDING_TEST)

build: $(BINDING_TEST)

format:
	$(CLANG_FORMAT) -i $(FORMAT_FILES)

$(OBJ_DIR)/%.o: %.cpp $(wildcard *.hpp) $(wildcard memory/*.hpp) $(wildcard runtime/*.hpp)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I. -I$(ROOT_DIR)/third_party -c $< -o $@

$(BINDING_TEST): $(TEST_OBJECTS) $(CATCH_SRC)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I. -I$(ROOT_DIR)/third_party $(TEST_OBJECTS) $(CATCH_SRC) -o $@ $(LDFLAGS)

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
